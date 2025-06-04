from flask import Flask, flash, redirect, render_template, request, session
from flask_session import Session
import os
import subprocess
from werkzeug.utils import secure_filename

# Configure application
app = Flask(__name__)

UPLOAD_FOLDER = 'uploads/raw'  # Setting where user files are saved

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16 MB max file size

# Allowed file extensions for security
ALLOWED_EXTENSIONS = {'mp3'}

def allowed_file(filename):
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

@app.route("/", methods=['GET', 'POST'])
def index():
    """The main page of the website"""
    if request.method == 'POST':
        file = request.files['file']
        # TODO i should validate that it is an mp3 file.
        # Really think trough the security aspect of receiving an unknown file to your server.
        file.save('uploads/raw/' + file.filename)
        return render_template("index.html")
    return render_template("index.html")
    



# This line should run the debugger
if __name__ == "__main__":
    app.run(debug=True)