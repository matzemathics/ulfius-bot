FROM archlinux

RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm base-devel ulfius cmake libsodium

COPY . app
WORKDIR /app
RUN rm -rf build && mkdir build && cmake . -B build && cmake --build build

EXPOSE 8080
ENTRYPOINT ["/app/build/discord-bot"]