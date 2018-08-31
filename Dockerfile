FROM debian:jessie

RUN apt-get update
RUN apt-get install -y autoconf-archive automake-1.14 autoconf2.59 build-essential check libspatialindex-dev libhiredis-dev libgcrypt11-dev libcurl4-openssl-dev

RUN mkdir /geotaxi/
WORKDIR /geotaxi/

COPY makefile .
COPY lib/ lib
COPY src/ src
COPY wait-for-it.sh wait-for-it.sh

RUN chmod +x wait-for-it.sh

RUN mkdir obj
RUN make

EXPOSE 8080
CMD ["./geoloc-server", "-p", "8080"]
