from flask import Flask, request, jsonify
app = Flask(__name__)

@app.route('/', methods=['POST'])
def data():
    print("Received JSON:", request.json)
    return jsonify({"status": "ok", "received": request.json})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
