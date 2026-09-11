"""
# This might be something I revisit in future? 
# Issue is I realised if the aim is to generate many messages that an engine should be parsing, then python is just slower than C++
# and I'll need to load it all into a vector anyway, so why not do it in a C++ file that loads the vector before beginning the benchmark?
# Python will be used for visuals I guess

# Create some typa script which generates rows on rows on rows of data given a parameter
import random
import argparse

def genMessages(filename, totalMessages, seed, price):
    output = open("data/formatted_data.csv", "w")


    with open(file, "r") as input:
        for line in input:
                output.write(line)
      

def main():
    parser = argparse.ArgumentParser(description="Generate a set of LOB messages")
    parser.add_argument("--o", help="What you wish the output file to be named")
    parser.add_argument("--total", help="How many messages you wish to be generated")
    parser.add_argument

if __name__ == "__main__":
      main()

"""