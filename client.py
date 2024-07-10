import socket
import os
import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext, ttk

# Constants
PORT = 8080
BUFFER_SIZE = 8192
SERVER_ADDRESS = '127.0.0.1'
BG_COLOR = '#ADD8E6'  # Light blue color

class AdminClientApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Client")
        self.root.configure(bg=BG_COLOR)
        self.sock = None
        self.authenticated = False

        # Username and Password fields
        self.username_label = tk.Label(root, text="Username:", bg=BG_COLOR, font=('Helvetica', 12, 'bold'))
        self.username_label.pack(pady=(10, 5))
        self.username_entry = tk.Entry(root, width=30, bg='white', font=('Helvetica', 12))
        self.username_entry.pack(pady=5, padx=20)
        
        self.password_label = tk.Label(root, text="Password:", bg=BG_COLOR, font=('Helvetica', 12, 'bold'))
        self.password_label.pack(pady=5)
        self.password_entry = tk.Entry(root, width=30, show='*', bg='white', font=('Helvetica', 12))
        self.password_entry.pack(pady=5, padx=20)
        
        self.login_button = tk.Button(root, text="Login", command=self.login, bg='#1E90FF', fg='white', font=('Helvetica', 12, 'bold'))
        self.login_button.pack(pady=15)

        # Dropdown menu for command selection
        self.command_var = tk.StringVar()
        self.command_var.set("")  # Set default value to empty

        self.command_menu = ttk.Combobox(root, textvariable=self.command_var, font=('Helvetica', 12), width=28)
        self.command_menu['values'] = ("CONVERT", "STATUS", "EXIT")
        self.command_menu.bind("<<ComboboxSelected>>", self.on_command_select)
        self.command_menu.pack(pady=10, padx=20)
        
        # Set style for the Combobox
        style = ttk.Style()
        style.configure('TCombobox', fieldbackground=BG_COLOR, background=BG_COLOR, font=('Helvetica', 12))

        # Button to send the command
        self.send_button = tk.Button(root, text="Send Command", command=self.send_command, bg='#1E90FF', fg='white', font=('Helvetica', 12, 'bold'))
        self.send_button.pack(pady=5)

        # Scrolled text area for displaying server responses
        self.output_text = scrolledtext.ScrolledText(root, width=60, height=20, bg='white', font=('Helvetica', 12))
        self.output_text.pack(pady=10, padx=20)

        # Connect to the server when the application starts
        self.connect_to_server()

    def connect_to_server(self):
        """
        Connect to the server.
        """
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((SERVER_ADDRESS, PORT))
            self.output_text.insert(tk.END, "Connected to server\n")
        except socket.error as e:
            self.output_text.insert(tk.END, f"Socket error: {e}\n")
            self.sock = None

    def login(self):
        """
        Send login credentials to the server.
        """
        username = self.username_entry.get()
        password = self.password_entry.get()
        if not username or not password:
            messagebox.showwarning("Warning", "Please enter both username and password")
            return
        login_command = f"LOGIN {username} {password}"
        self.sock.sendall(login_command.encode())
        response = self.sock.recv(BUFFER_SIZE).decode()
        if response == "Login successful":
            self.authenticated = True
            self.output_text.insert(tk.END, "Login successful\n")
            self.command_menu['values'] = ("CONVERT", "STATUS", "EXIT", "DISCONNECT", "UPTIME", "SHUTDOWN")
            self.login_button.pack_forget()
            self.username_label.pack_forget()
            self.username_entry.pack_forget()
            self.password_label.pack_forget()
            self.password_entry.pack_forget()
            self.root.title("Client")
        else:
            messagebox.showerror("Error", "Login failed")

    def on_command_select(self, event):
        """
        Handle command selection from the dropdown menu.
        """
        selected_command = self.command_var.get()
        if selected_command == "CONVERT":
            self.open_convert_window()
        elif selected_command == "DISCONNECT":
            self.prompt_disconnect()

    def send_convert_command(self, command, file_path, json_name):
        """
        Send a CONVERT command with XML data to the server.
        """
        with open(file_path, 'r') as xml_file:
            xml_data = xml_file.read()
            command += f' {json_name}\n' + xml_data

        self.sock.sendall(command.encode())
        self.output_text.insert(tk.END, f"Command sent to server: {command}\n")
        self.receive_response(json_path=f"{json_name}.json")

    def open_convert_window(self):
        """
        Open a new window to select the file for conversion.
        """
        self.convert_window = tk.Toplevel(self.root)
        self.convert_window.title("Select File for CONVERT")
        self.convert_window.configure(bg=BG_COLOR)

        # Label and entry for the file path
        tk.Label(self.convert_window, text="File path for CONVERT:", bg=BG_COLOR, font=('Helvetica', 12, 'bold')).pack(pady=5)
        self.file_path_entry = tk.Entry(self.convert_window, width=30, bg='white', font=('Helvetica', 12))
        self.file_path_entry.pack(pady=5, padx=20)

        # Button to browse for the file
        browse_button = tk.Button(self.convert_window, text="Browse", command=self.browse_file, bg='#1E90FF', fg='white', font=('Helvetica', 12, 'bold'))
        browse_button.pack(pady=5)

        # Label and entry for the JSON file name
        tk.Label(self.convert_window, text="JSON file name:", bg=BG_COLOR, font=('Helvetica', 12, 'bold')).pack(pady=5)
        self.json_name_entry = tk.Entry(self.convert_window, width=30, bg='white', font=('Helvetica', 12))
        self.json_name_entry.pack(pady=5, padx=20)

        # Button to confirm the conversion
        confirm_button = tk.Button(self.convert_window, text="Confirm", command=self.confirm_conversion, bg='#1E90FF', fg='white', font=('Helvetica', 12, 'bold'))
        confirm_button.pack(pady=5)

    def browse_file(self):
        """
        Open a file dialog to select an XML file.
        """
        file_path = filedialog.askopenfilename(filetypes=[("XML files", "*.xml")])
        if file_path:
            self.file_path_entry.insert(0, file_path)

    def confirm_conversion(self):
        """
        Confirm the conversion and send the command to the server.
        """
        file_path = self.file_path_entry.get()
        json_name = self.json_name_entry.get()
        if not file_path:
            messagebox.showwarning("Warning", "Please select a file path for CONVERT command")
            # self.open_convert_window()
            return
        if not json_name:
            messagebox.showwarning("Warning", "Please enter a name for the JSON file")
            # self.open_convert_window()
            return
        self.send_convert_command("CONVERT", file_path, json_name)
        self.convert_window.destroy()

    def prompt_disconnect(self):
        """
        Prompt for the user ID to disconnect or '*' to disconnect all users.
        """
        popup = tk.Toplevel(self.root)
        popup.title("DISCONNECT")
        popup.configure(bg=BG_COLOR)
        tk.Label(popup, text="Enter user ID or * to disconnect all:", bg=BG_COLOR, font=('Helvetica', 12, 'bold')).pack(pady=5)
        self.disconnect_entry = tk.Entry(popup, width=30, bg='white', font=('Helvetica', 12))
        self.disconnect_entry.pack(pady=5, padx=20)
        tk.Button(popup, text="Confirm", command=lambda: self.confirm_disconnect(popup), bg='#1E90FF', fg='white', font=('Helvetica', 12, 'bold')).pack(pady=5)

    def confirm_disconnect(self, popup):
        """
        Confirm the disconnection and send the command to the server.
        """
        user_id = self.disconnect_entry.get()
        popup.destroy()
        if user_id == "*" or user_id.isdigit():
            self.send_generic_command(f"DISCONNECT {user_id}")
        else:
            messagebox.showwarning("Warning", "Please enter a valid user ID or '*'")

    def send_command(self):
        """
        Send a command to the server.
        """
        command = self.command_var.get()
        
        if command == "CONVERT":
            # Ensure file_path and json_name are set before sending the command
            if not hasattr(self, 'file_path') or not self.file_path:
                messagebox.showwarning("Warning", "Please ensure a file path is provided for CONVERT command")
                # self.open_convert_window()
                return
            if not hasattr(self, 'json_name') or not self.json_name:
                messagebox.showwarning("Warning", "Please ensure a JSON file name is provided for CONVERT command")
                # self.open_convert_window()
                return
            
            self.send_convert_command(command, self.file_path, self.json_name)
        
        elif command == "EXIT":
            self.sock.sendall(command.encode())
            self.root.quit()  # Close the main window
        else:
            self.send_generic_command(command)

    def send_convert_command(self, command, file_path, json_name):
        """
        Send a CONVERT command with XML data to the server.
        """
        with open(file_path, 'r') as xml_file:
            xml_data = xml_file.read()
            command += '\n' + xml_data

        self.sock.sendall(command.encode())
        self.output_text.insert(tk.END, f"Command sent to server: {command}\n")
        self.receive_response(json_path=f"{json_name}.json")

    def send_generic_command(self, command):
        """
        Send a generic command to the server.
        """
        self.sock.sendall(command.encode())
        self.output_text.insert(tk.END, f"Command sent to server: {command}\n")
        self.receive_response()

    def receive_response(self, json_path=None):
        """
        Receive and display the server's response. Save to a JSON file if provided.
        """
        buffer = self.sock.recv(BUFFER_SIZE).decode()
        if buffer:
            self.output_text.insert(tk.END, f"Received response from server:\n{buffer}\n")
            if json_path:
                with open(json_path, 'w') as json_file:
                    json_file.write(buffer)
                    self.output_text.insert(tk.END, f"JSON saved to {json_path}\n")
        else:
            self.output_text.insert(tk.END, "Failed to receive response from server.\n")

if __name__ == "__main__":
    # Create the main application window
    root = tk.Tk()
    app = AdminClientApp(root)
    # Start the main event loop
    root.mainloop()

