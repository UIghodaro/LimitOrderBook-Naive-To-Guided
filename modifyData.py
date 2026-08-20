# I downloaded some data which contains executions as a message, need to slime them out using a script
file = "AAPL_2012-06-21_34200000_57600000_message_5.csv"

input = open(file, "r")
output = open("formatted_data.csv", "w")


with open(file, "r") as input:
    for line in input:
            output.write(line)