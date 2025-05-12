FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# === System dependencies + Clang/LLVM + Ninja ===
RUN apt-get update && apt-get install -y \
    clang \
    lld \
    llvm \
    libc++-dev \
    libc++abi-dev \
    build-essential \
    cmake \
    ninja-build \
    libmysqlclient-dev \
    wget \
    git \
    curl \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# === Set Clang as default compiler ===
ENV CC=clang
ENV CXX=clang++

# === Install CMake from source ===
ARG CMAKE_VER=3.27.9
WORKDIR /tmp/cmake
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VER}/cmake-${CMAKE_VER}.tar.gz \
    && tar -xzf cmake-${CMAKE_VER}.tar.gz \
    && cd cmake-${CMAKE_VER} \
    && ./bootstrap --parallel=$(nproc) \
    && make -j$(nproc) \
    && make install \
    && cd / && rm -rf /tmp/cmake


# === Build Boost 1.88.0 ===
ARG BOOST_VER=1.88.0
WORKDIR /tmp/boost
RUN wget https://github.com/boostorg/boost/releases/download/boost-${BOOST_VER}/boost-${BOOST_VER}-b2-nodocs.tar.gz \
    && tar xf boost-${BOOST_VER}-b2-nodocs.tar.gz \
    && cd boost-${BOOST_VER} \
    && ./bootstrap.sh \
    && m=$(nproc) && m=$(( m > 4 ? m - 2 : m )) && m=$(( m < 2 ? 2 : m )) \
    && ./b2 -j${m} install toolset=clang \
    && cd / && rm -rf /tmp/boost

# === Clone jh-toolkit from LTS branch ===
ARG JH_BRANCH=1.3.x-LTS
WORKDIR /tmp/build
RUN git clone --branch ${JH_BRANCH} --depth=1 https://github.com/JeongHan-Bae/jh-toolkit.git

# === Build jh-toolkit POD module ===
WORKDIR /tmp/build/jh-toolkit
RUN cmake -G Ninja -B build-pod -DCMAKE_BUILD_TYPE=Release -DTAR=POD \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    && cmake --build build-pod \
    && cmake --install build-pod

# === Copy and build TodoBuild ===
WORKDIR /tmp/build/TodoBuild
COPY Application /tmp/build/TodoBuild/Application
COPY Entity /tmp/build/TodoBuild/Entity
COPY Persistence /tmp/build/TodoBuild/Persistence
COPY Web /tmp/build/TodoBuild/Web
COPY main.cpp /tmp/build/TodoBuild/main.cpp
COPY CMakeLists.txt /tmp/build/TodoBuild/CMakeLists.txt

WORKDIR /tmp/build/TodoBuild
RUN test -f CMakeLists.txt \
    && cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    && cmake --build build

# === Package artifact ===
RUN mkdir -p /usr/app \
    && mv build/TodoAPP /usr/app/TodoAPP

# === Cleanup ===
RUN rm -rf /tmp/build

WORKDIR /usr/app
EXPOSE 8080
CMD ["./TodoAPP"]
