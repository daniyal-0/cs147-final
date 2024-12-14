from flask import Flask, request
import json

app = Flask(__name__)

@app.route("/")
def hello():
    #Retrieve parameter from request
    data_param = request.args.get("data")
    
    if data_param:
        try:
            #Parse JSON string into dictionary
            data = json.loads(data_param)
            
            #Print using f-strings
            print(f"Temperature: {data.get('temp')}°C")
            print(f"Humidity: {data.get('humidity')}%")
            print(f"Temperature Avg: {data.get('temp_avg')}°C")
            print(f"Temperature Min: {data.get('temp_min')}°C")
            print(f"Temperature Max: {data.get('temp_max')}°C")
            print(f"Humidity Avg: {data.get('hum_avg')}%")
            print(f"Humidity Min: {data.get('hum_min')}%")
            print(f"Humidity Max: {data.get('hum_max')}%")
            
            return f"Data received: {json.dumps(data)}"
        except json.JSONDecodeError:
            #Address decoding errors
            print("Invalid JSON received.")
            return "Invalid JSON format", 400
    else:
        #If there is no data
        print("No data received.")
        return "No data parameter provided", 400

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
