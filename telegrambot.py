import requests

bot_token = '793020144:AAH5B86xw9eZLIyCFCQVv6HniNaoOxkhJpA'
chat_id = '5716036793'
message = 'Hello Sahil! This is a message from ESP32 bot.'

url = f'https://api.telegram.org/bot{bot_token}/sendMessage'
params = {
    'chat_id': chat_id,
    'text': message
}

response = requests.get(url, params=params)
print(response.json())
