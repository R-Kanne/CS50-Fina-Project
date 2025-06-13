from flask import Flask, jsonify, redirect, render_template, request, session
from flask_session import Session
import os
import subprocess
from werkzeug.utils import secure_filename
import json

# Configure application
app = Flask(__name__)

UPLOAD_FOLDER = 'uploads/raw'  # Setting where user files are saved

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16 MB max file size

# Allowed file extensions for security
ALLOWED_EXTENSIONS = {'mp3'}

def allowed_file(filename):
    """ Return True if filename has an allowed extension."""
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

@app.route("/", methods=['GET', 'POST'])
def index():
    """The main page of the website, handles file uploads and analysis."""
    if request.method == 'POST':
         if 'file' not in request.files: # Assuming the input field name is 'mp3_file'
            return jsonify({"error": "No file part in the request"}), 400
         
         file = request.files['file']  # Getting the submitted file

         if file and allowed_file(file.filename):
            # Securely save the filename to prevent directory traversal attacks
            filename = secure_filename(file.filename)
            filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
            file.save(filepath)

            try:
                result = subprocess.run(
                    ["./scripts/analyze", filepath],
                    capture_output=True,
                    text=True,
                    check=True
                )
                output = result.stdout
            except subprocess.CalledProcessError as e:
                output = f"Error: {e.stderr}"



         # TODO i should validate that it is an mp3 file.
         # Really think trough the security aspect of receiving an unknown file to your server.
         return render_template("index.html")
    return render_template("index.html")
    



# This line should run the debugger
if __name__ == "__main__":
    app.run(debug=True)