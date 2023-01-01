FROM fedora:35
COPY . /usr/src/malloc-inspector
WORKDIR /usr/src/malloc-inspector
RUN sudo dnf install -y strace gcc make findutils
RUN make
CMD ["./run.sh"]
