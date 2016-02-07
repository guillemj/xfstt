FROM debian:buster

WORKDIR /build

# We need the dependencies for the travis script, and build-essential,
# and then the build-dependencies for the package.
RUN apt update -qq \
    && apt install --no-install-recommends -y \
      git wget curl ruby \
    && apt install --no-install-recommends -y \
      autoconf automake autopoint gettext \
      make pkg-config x11proto-fonts-dev \
      g++ \
    $EOL
