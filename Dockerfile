FROM ubuntu:18.04

RUN apt update && apt install -y libncursesw5-dev libncurses5-dev libreadline-dev gcc

WORKDIR /mnt
