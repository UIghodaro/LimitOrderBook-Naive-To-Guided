# The exact version of gcc running on the main laptop I use... I should probably update
FROM gcc:11.4.0

WORKDIR /app
COPY . .

# Main command for compilation
RUN g++ -O3 -o benchmark.out benchmark.cpp

CMD ["./benchmark.out", "--validate-only"]