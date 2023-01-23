FROM archlinux

RUN pacman -Syu --noconfirm base-devel ulfius cmake libsodium

COPY . app
WORKDIR /app
RUN rm -rf build && mkdir build && cmake . -B build && cmake --build build

# Run tests
RUN cd build && make test

EXPOSE 8080
ENTRYPOINT ["/app/build/discord-bot"]
