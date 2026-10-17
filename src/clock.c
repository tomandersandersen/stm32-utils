#include "clock.h"

#include "stm32f407xx.h"

#define CLOCK_TIMEOUT 1000000U

#define HSE_HZ 8000000U

static uint32_t calculate_core_clock_hz(void)
{
    uint32_t pllcfgr = RCC->PLLCFGR;
    uint32_t cfgr = RCC->CFGR;

    uint32_t pllm = pllcfgr & RCC_PLLCFGR_PLLM;
    uint32_t plln = (pllcfgr & RCC_PLLCFGR_PLLN) >>
                    RCC_PLLCFGR_PLLN_Pos;
    uint32_t pllp = (((pllcfgr & RCC_PLLCFGR_PLLP) >>
                      RCC_PLLCFGR_PLLP_Pos) + 1U) * 2U;

    uint32_t hpre = (cfgr & RCC_CFGR_HPRE) >>
                    RCC_CFGR_HPRE_Pos;

    static const uint16_t ahb_dividers[16] = {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 2, 4, 8, 16, 64, 128, 256
    };

    uint32_t sysclk_hz =
        (HSE_HZ / pllm) * plln / pllp;

    return sysclk_hz / ahb_dividers[hpre];
}

bool clock_init(void)
{
    uint32_t timeout;

    /*
     * 1. Enable the external high-speed oscillator (HSE).
     */
    RCC->CR |= RCC_CR_HSEON;

    timeout = CLOCK_TIMEOUT;
    while ((RCC->CR & RCC_CR_HSERDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /*
     * 2. Configure Flash for 48 MHz operation.
     *    One wait state is required at this frequency.
     */
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_1WS |
                  FLASH_ACR_PRFTEN |
                  FLASH_ACR_ICEN |
                  FLASH_ACR_DCEN;

    /*
     * 3. Configure the PLL.
     *
     *    HSE  = 8 MHz
     *    PLLM = 8  -> PLL input = 1 MHz
     *    PLLN = 192 -> VCO = 192 MHz
     *    PLLP = 4  -> SYSCLK = 48 MHz
     *    PLLQ = 4  -> USB clock = 48 MHz
     */
    RCC->CR &= ~RCC_CR_PLLON;

    timeout = CLOCK_TIMEOUT;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    RCC->PLLCFGR =
        (8U   << RCC_PLLCFGR_PLLM_Pos) |
        (192U << RCC_PLLCFGR_PLLN_Pos) |
        (1U   << RCC_PLLCFGR_PLLP_Pos) | /* PLLP encoding 01 = /4 */
        RCC_PLLCFGR_PLLSRC_HSE |
        (4U   << RCC_PLLCFGR_PLLQ_Pos);

    /*
     * 4. Configure bus prescalers.
     *
     *    AHB  = SYSCLK / 1 = 48 MHz
     *    APB1 = HCLK   / 2 = 24 MHz
     *    APB2 = HCLK   / 1 = 48 MHz
     */
    RCC->CFGR &= ~((0xFU << RCC_CFGR_HPRE_Pos) |
                   (7U   << RCC_CFGR_PPRE1_Pos) |
                   (7U   << RCC_CFGR_PPRE2_Pos));

    RCC->CFGR |= (4U << RCC_CFGR_PPRE1_Pos);

    /*
     * 5. Enable the PLL.
     */
    RCC->CR |= RCC_CR_PLLON;

    timeout = CLOCK_TIMEOUT;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }

    /*
     * 6. Switch SYSCLK to the PLL.
     */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    timeout = CLOCK_TIMEOUT;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
        if (--timeout == 0U) {
            return false;
        }
    }

    return true;
}

uint32_t clock_get_core_hz(void) {
    return calculate_core_clock_hz();
}