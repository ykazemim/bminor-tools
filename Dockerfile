FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends cmake g++ make
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF && cmake --build build

FROM ubuntu:24.04
COPY --from=build /src/build/bminor /usr/local/bin/bminor
WORKDIR /work
ENTRYPOINT ["bminor"]
