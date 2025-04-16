from flask import Flask, flash, redirect, render_template, request, session
from flask_session import Session

# Configure application
app = Flask(__name__)





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