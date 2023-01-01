FROM fedora:35
RUN sudo dnf install -y strace gcc make findutils
COPY . /usr/src/malloc-inspector
WORKDIR /usr/src/malloc-inspector
RUN make
CMD ["./run.sh"]
