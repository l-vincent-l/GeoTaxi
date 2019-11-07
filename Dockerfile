FROM debian:stretch

RUN apt-get update && apt-get install -y \
    autoconf-archive \
    automake \
    autoconf \
    build-essential \
    check \
    libspatialindex-dev \
    libhiredis-dev \
    libgcrypt11-dev \
    libcurl4-openssl-dev \
    wait-for-it

COPY . /geotaxi
WORKDIR /geotaxi/

RUN make

EXPOSE 8080/udp
CMD ["./geoloc-server", "-p", "8080"]
