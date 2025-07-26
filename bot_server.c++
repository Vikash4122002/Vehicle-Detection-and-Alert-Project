from telegram import Update
from telegram.ext import ApplicationBuilder, CommandHandler, MessageHandler, filters, ContextTypes
import requests

BOT_TOKEN = "7793020144:AAH5B86xw9eZLIyCFCQVv6HniNaoOxkhJpA"  # Replace with your Telegram Bot Token
GOOGLE_API_KEY = "AIzaSyDW-lzmt44_fVkTEw5ObBxaIvhVZow0HR8"  # Replace with your Google API key

async def start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    await update.message.reply_text("✅ Send me your GPS location and I'll find the nearest hospital.")

async def handle_location(update: Update, context: ContextTypes.DEFAULT_TYPE):
    lat = update.message.location.latitude
    lon = update.message.location.longitude

    # Get nearby hospitals
    url = f"https://maps.googleapis.com/maps/api/place/nearbysearch/json?location={lat},{lon}&radius=5000&type=hospital&key={GOOGLE_API_KEY}"
    response = requests.get(url).json()

    if "results" in response and len(response["results"]) > 0:
        hospital = response["results"][0]
        name = hospital["name"]
        place_id = hospital["place_id"]

        # Get hospital phone number
        details_url = f"https://maps.googleapis.com/maps/api/place/details/json?place_id={place_id}&fields=name,formatted_phone_number&key={GOOGLE_API_KEY}"
        detail_res = requests.get(details_url).json()
        phone = detail_res.get("result", {}).get("formatted_phone_number", "Not Available")

        if phone != "Not Available":
            await update.message.reply_text(f"📍 Nearest Hospital: {name}\n📞 Phone: {phone}")
        else:
            await update.message.reply_text(f"⚠️ Hospital found: {name}, but no phone number available.")
    else:
        await update.message.reply_text("❌ No hospital found nearby.")

# Build the app
app = ApplicationBuilder().token(BOT_TOKEN).build()
app.add_handler(CommandHandler("start", start))
app.add_handler(MessageHandler(filters.LOCATION, handle_location))

print("✅ Bot server running...")
app.run_polling()
