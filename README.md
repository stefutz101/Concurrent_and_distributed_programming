# Concurrent_and_distributed_programming

## Overview

This project implements a client-server application where the client can send XML files to the server for conversion into JSON format. The server performs the conversion and sends the JSON data back to the client. The client then saves the JSON data to a specified file. Additionally, the application supports various commands to manage the server and client interactions.

## Features

### Client

- **XML File Handling**: Reads XML files from a specified path.
- **Command Sending**: Sends commands to the server for various operations.
- **JSON File Handling**: Receives JSON data from the server and saves it to a specified file.
- **Commands**:
  - **CONVERT <path_to_xml>**: Converts an XML file to JSON.
  - **STATUS**: Checks the server status.
  - **SHUTDOWN**: Shuts down the server.
  - **EXIT**: Closes the client application.
  - **LOGIN <user> <pass>**: Authenticates the user.
  - **UPTIME**: Displays the server's uptime (available only for authenticated users).
  - **DISCONNECT <client_id>**: Disconnects a specific client (available only for authenticated users).
  - **DISCONNECT ***: Disconnects all clients (available only for authenticated users).

### Server

- **XML to JSON Conversion**: Parses XML files and converts them to JSON format.
- **Client Management**: Handles multiple client connections concurrently.
- **Command Handling**: Processes various commands sent by the client.
- **Authentication**: Manages user authentication using a credentials file.
- **Uptime Tracking**: Tracks and displays the server's uptime.

## Usage

1. **Start the Server**: Run the server application to start listening for client connections.
2. **Connect the Client**: Run the client application and connect to the server.
3. **Send Commands**: Use the client to send commands to the server as needed.
4. **Process Files**: Convert XML files to JSON and save the results.

## Example Commands

- `CONVERT /path/to/file.xml`
- `STATUS`
- `SHUTDOWN`
- `EXIT`
- `LOGIN username password`
- `UPTIME`
- `DISCONNECT 1`
- `DISCONNECT *`

## Installation

1. Clone the repository:
   ```sh
   git clone https://github.com/stefutz101/Concurrent_and_distributed_programming.git
   ```

2. Navigate to the project directory:
   ```sh
   cd Concurrent_and_distributed_programming
   ```
4. Compile the client and server:
   ```sh
   gcc -o server server.c -lxml2 -lcjson -lpthread
   gcc -o client client.c -lxml2 -lcjson -lpthread
   ```

## Running the Application
1. Start the server:
   ```sh
   ./server
   ```
2. Start the client:
   ```sh
   ./client
   ```
3. Start the Windows client:
   ```sh
   python client.py
   ```

## Contributing
Feel free to submit issues, fork the repository and send pull requests.

## License
This project is licensed under the MIT License. Feel free to modify the content as per your specific requirements or project details.
