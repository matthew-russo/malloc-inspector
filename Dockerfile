FROM gcc:12
COPY . /usr/src/malloc-inspector
WORKDIR /usr/src/malloc-inspector
RUN make
CMD ["./build/malloc-inspector"]
