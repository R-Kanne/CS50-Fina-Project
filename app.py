from flask import Flask, flash, redirect, render_template, request, session
from flask_session import Session

# Configure application
app = Flask(__name__)





@app.route("/")
def index():
    """The main page of the website"""
    return render_template("index.html")