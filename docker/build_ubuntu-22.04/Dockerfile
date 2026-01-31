FROM ubuntu:22.04

# Prevent interactive prompts during apt-get
ENV DEBIAN_FRONTEND=noninteractive

RUN \
    dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y \
        gpg \
        wget \
        software-properties-common \
    && \
    wget -O- https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
    gpg --dearmor - | \
    tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | \
    tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt-get update && \
    apt-get install -y \
        cmake \
        ninja-build \
        clang-14 \
        gcc-11-multilib \
        g++-11-multilib \
        libstdc++6:i386 \
        libc6:i386 \
        git \
    && \
    rm -rf /var/lib/apt/lists/* && \
    useradd -m -u 1000 user

USER user

ENV CC=/usr/bin/clang-14 \
    CXX=/usr/bin/clang++-14 \
    PATH=/home/user/.local/bin:${PATH}

COPY --chown=user:user docker-entrypoint.sh /
ENTRYPOINT ["/docker-entrypoint.sh"]
