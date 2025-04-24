#include <ESPAsyncWebServer.h>
#include <time.h>
#include "DHT.h" // For DHT11 sensor

struct tm timeinfo;
extern DHT dht;

void protoRemote(AsyncResponseStream *response) {
    response->print("<!DOCTYPE html><html lang='en'>");
    response->print("<meta name='viewport' content='width=device-width'>");
  // Style/CSS Section
    response->print("<head><meta charset='utf-8'>");
    response->print("<title>Ardu4Weather</title>");
    response->print("<link rel='preconnect' href='https://fonts.googleapis.com'>");
    response->print("<link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>");
    response->print("<link href='https://fonts.googleapis.com/css2?family=Jersey+10&display=swap' rel='stylesheet'>");
    response->print("<link rel='stylesheet' href='https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200&icon_names=all_inclusive' />");
    response->print("<link rel='icon' href='https://i.imgur.com/mlL3Fiw.png'>");
    response->print("<style>");
    response->print("nav { background-color:rgb(0, 0, 0); transition-duration: 0.4s; font-size: 16px; display: flex;  align-items: center; padding: 10px 0; position: relative; }");
    response->print("nav:hover { background-color:rgb(255, 255, 255); transition-duration: 0.4s; font-size: 18px; display: flex; align-items: center; position: relative; text-color: rgb(0, 0, 0); }");
    response->print("nav ul { list-style: none; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; flex-grow: 1; }");
    response->print("nav li { display: inline-block; margin: 0 15px; }");
    response->print("nav a { color:rgb(255, 255, 255); text-decoration: none; }");
    response->print("nav a:hover { color:rgb(0, 0, 0); text-decoration: none; }");
    response->print("body { background: linear-gradient(0deg, rgb(0, 0, 0) 0%, rgb(44, 44, 44) 100%); font-family: 'Jersey 10', sans-serif; font-weight: 400; font-style: normal; margin: 0; /* Remove default margins */ display: flex; flex-direction: column; min-height: 100vh; /* Ensure full viewport height */ transition-duration: 0.4s; }");
    response->print("h1 { color: #ffffff; text-align: center; margin-bottom: 20px; font-size: 40px; }");
    response->print(".header-image img { max-width: 4%; height: auto; padding-left: 10px; display: block; float: left; }");
    response->print("button { background-color: rgba(255, 255, 255, 0.1); color:rgb(199, 199, 199); border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 10px; padding: 10px 20px; cursor: pointer; display: block; /* Make it a block element */ margin: 0 auto; /* Center horizontally */ transition-duration: 0.2s; font-family: inherit; }");
    response->print("button:hover { background-color: rgba(255, 255, 255, 0.1); color:rgb(255, 255, 255); border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 10px; padding: 10px 20px; cursor: pointer; display: block; /* Make it a block element */ margin: 0 auto; /* Center horizontally */ transition-duration: 0.2s; font-family: inherit; }");
    response->print("button:active { background-color:rgb(92, 92, 92); color: #ffffff; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; display: block; /* Make it a block element */ margin: 0 auto; }");
    response->print("footer { background-color: #000000; color: white; text-align: center; padding: 1px 0; margin-top: auto; transition-duration: 0.4s; }");
    response->print(".unit-container { display: flex; flex-wrap: wrap; /* Allow units to wrap to next line */ justify-content: space-around; /* Distribute space */ gap: 15px; /* Space between units */ margin-top: 20px; }");
    response->print(".unit { background-color: rgba(255, 255, 255, 0.1); /* Semi-transparent white/grey */ border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 10px; /* Rounded corners */ padding: 15px; flex: 1 1 200px; /* Allow units to grow/shrink, base width 200px */ min-width: 180px; /* Don't get too narrow */ box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2); text-align: center; /* Center text */ transition: background-color 0.3s; }");
    response->print(".unit:hover { background-color: rgba(255, 255, 255, 0.15); }"); // Slight hover effect
    response->print(".unit h3 { margin-top: 0; margin-bottom: 10px; font-size: 1.4em; color: #eee; }"); // Unit title style
    response->print(".unit p { margin-bottom: 0; font-size: 1.8em; font-weight: bold; color: #fff; }"); // Unit value style
    response->print(".unit .small-text { font-size: 0.9em; color: #ccc; margin-top: 5px; }"); // Optional smaller text style
    response->print("</style>");
    response->print("<nav>");
    response->print("<ul><li><a>Remote</a></li>");
    response->print("</ul></nav>");
    response->print("</head>");
    // Javascript Section
    // Body/HTML Section
    response->print("<body><h1>ProtoRemote</h1>");
    response->print("<div class='unit-container'>"); // Start the container for all units

    // Unit 1: Time
    response->print("<div class='unit'>");
    response->print("<h3>Time</h3>");
    response->print("<p id='time-value'>");
    if (getLocalTime(&timeinfo)) { // Check if fetching time was successful
      char timeBuffer[9];
      strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeinfo); // Format the time
      response->print(timeBuffer);
    } else {
      response->print("--:--:--");
    }
    response->print("</p>");
    response->print("</div>");

    // Unit 2: Battery Percentage
    response->print("<div class='unit'>");
    response->print("<h3>Battery</h3>");
    response->print("<p id='battery-value'>--%</p>"); // Placeholder
    response->print("<p class='small-text'>(Estimate)</p>"); // Example small text
    response->print("</div>");

    // Unit 3: Temperature
    response->print("<div class='unit'>");
    response->print("<h3>Temp (Internal)</h3>");
    response->print("<p id='temp-value'>");
    float temperature = dht.readTemperature();
    if (!isnan(temperature)) { // Check if the reading is valid
      response->print(temperature, 1); // Print with one decimal place
    } else {
      response->print("--"); // Placeholder for invalid reading
    }
    response->print(" °C</p>"); // Placeholder
    response->print("</div>");

    // Unit 4: Humidity
    response->print("<div class='unit'>");
    response->print("<h3>Humidity (Internal)</h3>");
    response->print("<p id='humidity-value'>");
    float humidity = dht.readHumidity();
    if (!isnan(humidity)) { // Check if the reading is valid
      response->print(humidity, 1); // Print with one decimal place
    } else {
      response->print("--"); // Placeholder for invalid reading
    }
    response->print(" %</p>");
    response->print("</div>");

    // Unit 5: Time Online
    response->print("<div class='unit'>"); // Fixed the class attribute
    response->print("<h3>Time Online</h3>");
    response->print("<p id='online-value'>");
    unsigned long uptime = millis() / 1000; // Get uptime in seconds
    unsigned long hours = uptime / 3600;
    response->print(uptime);
    response->print(" S (");
    response->print(hours);
    response->print(" H)</p>"); // Placeholder
    response->print("<p class='small-text'>Seconds</p>");
    response->print("</div>");

    // Unit 6: Animation Control
    response->print("<div class='unit'>");
    response->print("<h3>Animation</h3>");
    // Add buttons here later using JS or directly
    response->print("<button onclick=\"sendCommand('anim_play_idle')\">Play Idle</button>");
    response->print("<button onclick=\"sendCommand('anim_play_happy')\">Play Happy</button>");
    response->print("</div>");

    // Unit 7: Audio Control
    response->print("<div class='unit'>");
    response->print("<h3>Audio</h3>");
    // Add buttons/sliders here later
    response->print("<button onclick=\"sendCommand('play_song1')\">Play Song 1</button>");
    response->print("<button onclick=\"sendCommand('stop_song')\">Stop Music</button>");
    response->print("</div>");
    response->print("<script>");
    response->print("function sendCommand(cmd) {");
    response->print("  console.log('Sending command: ' + cmd);");
    response->print("  fetch('/action?command=' + cmd)");
    response->print("    .then(response => response.text())");
    response->print("    .then(data => { console.log('Server response: ' + data); /* Update UI? */ })");
    response->print("    .catch(error => console.error('Error:', error));");
    response->print("}");
    // Add functions to update values for time, battery etc. later if needed
    response->print("</script></div>");
    response->print("<footer><p>ProtoRemote Interface - Hosted on ESP32 - CS<br>");
    response->print("<a href='https://github.com/Aprocryphan/YourProjectRepo' style='text-decoration: underline; color: #aaa; display: inline-block; margin-top: 5px;' target='_blank'>");
    response->print("<img src='https://github.githubassets.com/images/modules/logos_page/GitHub-Mark.png' alt='GitHub Logo' style='width: 20px; height: 20px; margin-right: 5px;'>");
    response->print("GitHub</a></p></footer>");
    response->print("</body></html>");
}