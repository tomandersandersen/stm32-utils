
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

CMD ["make"]
