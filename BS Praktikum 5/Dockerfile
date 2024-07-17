# Use an official runtime as a parent image
FROM gcc:latest

# Set the working directory
WORKDIR /usr/src/app

# Copy the current directory contents into the container at /usr/src/app
COPY . .

# Compile the C++ program
RUN g++ -o myprogram main.cpp

# Specify the command to run on container start
CMD ["./myprogram"]

