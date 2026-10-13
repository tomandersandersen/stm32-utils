#include "usb_device.h"

#include "stm32f407xx.h"

#include "log.h"
#include "assert.h"

#define GPIO_AF10 10U

#define USB_IN_ENDPOINT_REGS \
    ((USB_OTG_INEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + 0x900U))

#define USB_OUT_ENDPOINT_REGS \
    ((USB_OTG_OUTEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + 0xB00U))

#define USB_FIFO0 \
    (*(__IO uint32_t *)(USB_OTG_FS_PERIPH_BASE + 0x1000U))

#define USB_DEVICE_REGS \
    ((USB_OTG_DeviceTypeDef *)(USB_OTG_FS_PERIPH_BASE + 0x800U))

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/** Read n bytes, or discard package. */
#define USB_RX_FIFO_READ_OR_DRAIN(data, n, pck_byte_count) \
    do                                                     \
    {                                                      \
        if (pck_byte_count == n)                           \
            usb_rx_fifo_recv(data, pck_byte_count);        \
        else                                               \
            usb_rx_fifo_drain(pck_byte_count);             \
    } while (0)

static const uint8_t g_usb_device_descriptor[18] = {
    18,         // bLength
    0x01,       // bDescriptorType: DEVICE
    0x00, 0x02, // bcdUSB: USB 2.0
    0x00,       // bDeviceClass: specified by interfaces
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    64,         // bMaxPacketSize0
    0x34, 0x12, // idVendor: 0x1234 (temporary test value)
    0x78, 0x56, // idProduct: 0x5678 (temporary test value)
    0x00, 0x01, // bcdDevice: 1.00
    0,          // iManufacturer: no string descriptor
    0,          // iProduct: no string descriptor
    0,          // iSerialNumber: no string descriptor
    1           // bNumConfigurations
};

static void
usb_rx_fifo_configure(void)
{
    /* Configure the RX FIFO to 128 32-bit words (512 bytes). */
    USB_OTG_FS->GRXFSIZ = 128U;
}

static void usb_tx_fifos_configure(void)
{
    /* EP1 IN: start at word 192, depth 16 words (64 bytes). */
    USB_OTG_FS->DIEPTXF[0] = (192U << 16) | 16U;

    /* EP2 IN: start at word 208, depth 64 words (256 bytes). */
    USB_OTG_FS->DIEPTXF[1] = (208U << 16) | 64U;
}

static void usb_ep0_tx_fifo_configure(void)
{
    /* EP0 IN: start at word 128, depth 64 words (256 bytes). */
    USB_OTG_FS->DIEPTXF0_HNPTXFSIZ = (128U << 16) | 64U;
}

static void usb_device_configure(void)
{
    /* Configure full-speed device operation: DSPD = 00. */
    USB_DEVICE_REGS->DCFG &= ~USB_OTG_DCFG_DSPD_Msk;

    /* Start with USB address 0, as required before enumeration. */
    USB_DEVICE_REGS->DCFG &= ~USB_OTG_DCFG_DAD_Msk;
}

static bool usb_core_reset(void)
{
    uint32_t timeout = 1000000U;

    /* Wait until the AHB master is idle. */
    while ((USB_OTG_FS->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL) == 0U)
    {
        if (--timeout == 0U)
            return false;
    }

    /* Request a core soft reset. */
    USB_OTG_FS->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;

    timeout = 1000000U;

    /* Wait for the reset bit to clear automatically. */
    while ((USB_OTG_FS->GRSTCTL & USB_OTG_GRSTCTL_CSRST) != 0U)
    {
        if (--timeout == 0U)
            return false;
    }

    return true;
}

static void usb_force_device_mode(void)
{
    /* Force the OTG FS core into USB device mode. */
    USB_OTG_FS->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD;
}

static bool usb_wait_device_mode(void)
{
    uint32_t timeout = 1000000U;

    /* GINTSTS.CMOD = 0 means the core is in device mode. */
    while ((USB_OTG_FS->GINTSTS & 1U) != 0U)
    {
        if (--timeout == 0U)
            return false;
    }

    return true;
}

static void usb_phy_enable(void)
{
    /* Select the embedded full-speed PHY. */
    USB_OTG_FS->GUSBCFG |= USB_OTG_GUSBCFG_PHYSEL;

    /* Power up the internal USB transceiver. */
    USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_PWRDWN;

    /* Enable B-device VBUS sensing. */
    USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_VBUSBSEN;

    /* Ensure VBUS sensing is not globally disabled. */
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_NOVBUSSENS;
}

static void usb_ep0_init(void)
{
    /*
     * EP0 OUT:
     * - Full-speed control endpoint, 64-byte maximum packet.
     * - Reserve space for up to three 8-byte setup packets.
     */
    USB_OUT_ENDPOINT_REGS[0].DOEPTSIZ =
        (3U << 29U) |
        (1U << 19U) |
        24U;

    USB_OUT_ENDPOINT_REGS[0].DOEPCTL |=
        (1U << 15U) |
        (1U << 26U) |
        (1U << 31U);

    /* EP0 IN: activate the endpoint. */
    USB_IN_ENDPOINT_REGS->DIEPCTL |=
        (1U << 15U); /* USBAEP */
}

static void usb_interrupts_configure(void)
{
    /* Clear any stale USB reset status flag (write-one-to-clear). */
    USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_USBRST;

    /* Enable reset, enumeration done, and FIFO-not-empty interrupts. */
    USB_OTG_FS->GINTMSK |=
        USB_OTG_GINTMSK_USBRST |
        USB_OTG_GINTMSK_ENUMDNEM |
        USB_OTG_GINTMSK_RXFLVLM;

    /* Set the interrupt priority in the Cortex-M NVIC. */
    NVIC_SetPriority(OTG_FS_IRQn, 5);

    /* Enable the OTG FS interrupt in the NVIC. */
    NVIC_EnableIRQ(OTG_FS_IRQn);

    /* Enable the core's global interrupt output. */
    USB_OTG_FS->GAHBCFG |= USB_OTG_GAHBCFG_GINT;
}

static void usb_connect(void)
{
    /* Clear SDIS to allow the device to connect to the host. */
    USB_DEVICE_REGS->DCTL &= ~USB_OTG_DCTL_SDIS;
}

static void usb_ep0_send(const uint8_t *data, uint32_t length)
{
    ASSERT(length <= 64U);

    const uint32_t words = (length + 3U) / 4U;

    /* Ensure EP0's TX FIFO has room for the complete packet. */
    ASSERT((USB_IN_ENDPOINT_REGS[0].DTXFSTS & 0xFFFFU) >= words);

    /* Program one IN packet with the requested payload length. */
    USB_IN_ENDPOINT_REGS[0].DIEPTSIZ =
        (1U << 19U) | /* PKTCNT = 1 */
        length;       /* XFRSIZ */

    /* Enable EP0 IN and clear NAK. */
    USB_IN_ENDPOINT_REGS[0].DIEPCTL |=
        (1U << 15U) | /* USBAEP. TODO: this should probably be set on config. */
        (1U << 26U) | /* CNAK. Clear NACK.*/
        (1U << 31U);  /* EPENA. End point enable. */

    /* Write payload to TX FIFO 0 in 32-bit words. */
    for (uint32_t offset = 0U; offset < length; offset += 4U)
    {
        uint32_t word = 0U;

        for (uint32_t j = 0U; j < 4U; ++j)
        {
            const uint32_t index = offset + j;

            if (index < length)
                word |= (uint32_t)data[index] << (j * 8U);
        }

        USB_FIFO0 = word;
    }
}

static void usb_rx_fifo_recv(uint8_t *data, const uint32_t byte_count)
{
    const uint32_t words = (byte_count + 3U) / 4U;

    for (uint32_t i = 0U; i < words; ++i)
    {
        const uint32_t word = USB_FIFO0;
        const uint32_t remaining = byte_count - i * 4U;
        const uint32_t bytes = remaining < 4U ? remaining : 4U;

        for (uint32_t j = 0U; j < bytes; ++j)
        {
            data[i * 4U + j] = (uint8_t)(word >> (j * 8U));
        }
    }
}

static void usb_rx_fifo_drain(const uint32_t byte_count)
{
    const uint32_t words = (byte_count + 3U) / 4U;

    for (uint32_t i = 0U; i < words; ++i)
    {
        (void)USB_FIFO0;
    }
}

static void usb_rx_handle_pck_setup(uint32_t ep_num,
                                    uint32_t byte_count)
{
    uint8_t data[8] = {0};

    if (ep_num != 0U)
    {
        LOG_WARNING("SETUP packet received on nonzero endpoint");
        usb_rx_fifo_drain(byte_count);
        return;
    }

    if (byte_count != sizeof(data))
    {
        LOG_WARNING("Invalid SETUP packet length");
        usb_rx_fifo_drain(byte_count);
        return;
    }

    usb_rx_fifo_recv(data, sizeof(data));
    LOG_DEBUG_HEX("USB setup", data, sizeof(data));

    /* bmRequestType: Characteristics of the request:
     *   Bits 7:7  - Direction (0 = Host-to-device, 1 = Device-to-host)
     *   Bits 6:5  - Type (0 = Standard, 1 = Class, 2 = Vendor-specific)
     *   Bits 4:0  - Recipient (0 = Device, 1 = Interface, 2 = Endpoint)
     */
    const uint8_t bm_request_type = data[0];

    /* bRequest: Request identifier, e.g. 0x06 = GET_DESCRIPTOR. */
    const uint8_t b_request = data[1];

    /* wValue: Request-specific value.
     * For GET_DESCRIPTOR:
     *   Bits 15:8 - Descriptor type (0x01 = Device descriptor)
     *   Bits  7:0 - Descriptor index (0 = first descriptor)
     */
    const uint16_t w_value =
        (uint16_t)data[2] |
        ((uint16_t)data[3] << 8U);

    /* wIndex: Request-specific index.
     * For GET_DESCRIPTOR, identifies the language or interface
     * for certain descriptor types. Unused for a device descriptor.
     */
    const uint16_t w_index =
        (uint16_t)data[4] |
        ((uint16_t)data[5] << 8U);

    /* wLength: Maximum number of data bytes requested by the host.
     * The device must not send more than this amount.
     */
    const uint16_t w_length =
        (uint16_t)data[6] |
        ((uint16_t)data[7] << 8U);

    switch (b_request)
    {
    case 0x06U: /* GET_DESCRIPTOR */
        if (bm_request_type == 0x80U &&
            (w_value >> 8U) == 0x01U &&
            (w_value & 0xFFU) == 0U)
        {
            LOG_INFO("GET_DESCRIPTOR: device");
            usb_ep0_send(g_usb_device_descriptor, MIN(sizeof(g_usb_device_descriptor), w_length));
        }
        break;

    default:
        LOG_WARNING("Unsupported USB request");
        break;
    }

    (void)w_index;
    (void)w_length;
}

enum PckType : uint32_t
{
    PACKAGE_TYPE_SETUP = 6u
};

static void usb_rx_fifo_handle(void)
{
    while ((USB_OTG_FS->GINTSTS &
            USB_OTG_GINTSTS_RXFLVL) != 0U)
    {

        /*
         * USB OTG FS Receive Status Read and Pop Register (GRXSTSP)
         *
         *  31            21 20       17 16 15       15 14             4 3       0
         * +----------------+-----------+--+-----------+----------------+---------+
         * |   Reserved     | PKTSTS    |Reserved|    BCNT       | EPNUM   |
         * +----------------+-----------+--+-----------+----------------+---------+
         *
         * Bits  3:0   EPNUM  - Endpoint number associated with the event
         * Bits 14:4   BCNT   - Byte count; number of bytes in the received data
         * Bits 16:15  Reserved
         * Bits 20:17  PKTSTS - Packet status / receive event type
         * Bits 31:21  Reserved
         *
         * Relevant PKTSTS values:
         *   0x2 - OUT data packet received
         *   0x3 - OUT transfer completed
         *   0x4 - SETUP transaction completed
         *   0x6 - SETUP data packet received
         *
         * Reading GRXSTSP pops the next entry from the receive status queue.
         * The actual packet payload is read separately from the RX FIFO.
         */

        const uint32_t rx_status = USB_OTG_FS->GRXSTSP;
        const uint32_t ep_num = rx_status & 0xFU;
        const uint32_t byte_count = (rx_status >> 4U) & 0x7FFU;
        const uint32_t packet_status = (rx_status >> 17U) & 0xFU;

        if (packet_status == PACKAGE_TYPE_SETUP)
        {
            usb_rx_handle_pck_setup(ep_num, byte_count);
        }
        else
        {
            LOG_WARNING("Package status handler not implemented");
        }
    }
}

void OTG_FS_IRQHandler(void)
{
    uint32_t status = USB_OTG_FS->GINTSTS;

    if ((status & USB_OTG_GINTSTS_USBRST) != 0U)
    {
        LOG_INFO("USB reset");

        /* Acknowledge the write-one-to-clear interrupt. */
        USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_USBRST;

        /* Reset device address to zero. */
        USB_DEVICE_REGS->DCFG &= ~USB_OTG_DCFG_DAD_Msk;
    }
    if ((status & USB_OTG_GINTSTS_ENUMDNE) != 0U)
    {
        USB_OTG_FS->GINTSTS = USB_OTG_GINTSTS_ENUMDNE;

        LOG_INFO("USB enumeration done");
        usb_ep0_init();
    }
    if ((status & USB_OTG_GINTSTS_RXFLVL) != 0U)
    {
        usb_rx_fifo_handle();
    }
    if ((status & USB_OTG_GINTSTS_IEPINT) != 0U)
    {
        const uint32_t diepint = USB_IN_ENDPOINT_REGS[0].DIEPINT;

        if ((diepint & USB_OTG_DIEPINT_XFRC) != 0U)
        {
            /* Acknowledge transfer complete (write one to clear). */
            USB_IN_ENDPOINT_REGS[0].DIEPINT = USB_OTG_DIEPINT_XFRC;
            LOG_DEBUG("EP0 IN transfer complete");
        }
    }
}

bool usb_device_init(void)
{
    LOG_DEBUG("Initiating USB");

    /* Enable the USB OTG FS peripheral clock. */
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    (void)RCC->AHB2ENR;

    /* Enable GPIOA clock. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    /* Configure PA11 (D-) and PA12 (D+) as alternate-function pins. */
    GPIOA->MODER &= ~(GPIO_MODER_MODER11 | GPIO_MODER_MODER12);
    GPIOA->MODER |= GPIO_MODER_MODER11_1 | GPIO_MODER_MODER12_1;

    /* Select AF10 (USB OTG FS) for PA11 and PA12. */
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL11 | GPIO_AFRH_AFSEL12);
    GPIOA->AFR[1] |= (GPIO_AF10 << GPIO_AFRH_AFSEL11_Pos) |
                     (GPIO_AF10 << GPIO_AFRH_AFSEL12_Pos);

    /* Select the embedded PHY before resetting the core. */
    USB_OTG_FS->GUSBCFG |= USB_OTG_GUSBCFG_PHYSEL;

    if (!usb_core_reset())
        return false;

    /* Power up the PHY and configure VBUS sensing. */
    usb_phy_enable();

    /* Force device mode and wait until the core reports device mode. */
    usb_force_device_mode();

    if (!usb_wait_device_mode())
        return false;

    /* Configure device speed and initial USB address. */
    usb_device_configure();

    /* Configure the USB FIFO allocation. */
    usb_rx_fifo_configure();
    usb_ep0_tx_fifo_configure();
    usb_tx_fifos_configure();

    /* Enable reset interrupts and the global interrupt output. */
    usb_interrupts_configure();

    /* Allow the device to connect to the host. */
    usb_connect();

    return true;
}