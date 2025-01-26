#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <algorithm> 

// ======================
// Configuration
// ======================
static const char* AP_SSID = "PixelFlow";
static const char* JPG_FOLDER = "/"; // Folder containing JPG files

WebServer server(80);
bool serverRunning = false;
std::vector<String> imageFiles; // List of image files
size_t currentImageIndex = 0;

unsigned long lastImageChange = 0;
const char* CONFIG_FILE = "/config.json";
unsigned long IMAGE_CHANGE_INTERVAL = 75; // Valeur par défaut : 10 secondes

void loadConfig() {
  if (!SPIFFS.exists(CONFIG_FILE)) {
    Serial.println("Configuration file not found, using default settings.");
    return;
  }

  File file = SPIFFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("Failed to open configuration file.");
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Failed to parse configuration file, using default settings.");
    return;
  }

  IMAGE_CHANGE_INTERVAL = doc["imageChangeInterval"] | 10000; // 10s par défaut
  Serial.printf("Configuration loaded: IMAGE_CHANGE_INTERVAL = %lu ms\n", IMAGE_CHANGE_INTERVAL);
}

void saveConfig() {
  StaticJsonDocument<256> doc;
  doc["imageChangeInterval"] = IMAGE_CHANGE_INTERVAL;

  File file = SPIFFS.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("Failed to open configuration file for writing.");
    return;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to configuration file.");
  }

  file.close();
  Serial.println("Configuration saved.");
}

// ---------------------------------------------------------
// 1) Common HTML parts (Header and Footer + embedded style)
// ---------------------------------------------------------
const char HTML_HEADER[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>PixelFlow - Image Management</title>
  <style>
    body {
      background-color: #f0f8ff;
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      color: #333;
    }
    h1 {
      color: #444;
      text-align: center;
      margin-bottom: 20px;
    }
    a {
      color: #007bff;
      text-decoration: none;
    }
    a:hover {
      text-decoration: underline;
    }
    ul {
      list-style-type: none;
      padding: 0;
      margin: 0;
    }
    li {
      background: #ffffff;
      margin: 10px 0;
      padding: 15px;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .btn {
      background-color: #007bff;
      border: none;
      color: white;
      padding: 10px 20px;
      text-decoration: none;
      cursor: pointer;
      border-radius: 5px;
      font-size: 16px;
      transition: background-color 0.3s ease;
    }
    .btn:hover {
      background-color: #0056b3;
    }
    .btn.danger {
      background-color: #dc3545;
    }
    .btn.danger:hover {
      background-color: #b02a37;
    }
    .container {
      background: #ffffff;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      width: 90%;
      max-width: 600px;
      text-align: center;
    }
    .nav {
      margin-bottom: 20px;
    }
    .nav a {
      margin-right: 10px;
    }
    .warning {
      color: #d9534f;
      font-weight: bold;
    }
  </style>
</head>
<body>
<div class="container">
)=====";

const char HTML_FOOTER[] PROGMEM = R"=====(
</div>
</body>
</html>
)=====";

// --- Load available JPG images from SPIFFS ---
#include <algorithm> // Pour utiliser std::sort

void loadImageFiles() {
  imageFiles.clear();
  File root = SPIFFS.open(JPG_FOLDER);
  if (!root || !root.isDirectory()) {
    Serial.println("Error: Unable to open directory!");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    fileName.trim(); // Remove spaces or invisible characters
    if (fileName.startsWith("/")) {
      fileName = fileName.substring(1); // Remove the leading "/" if necessary
    }
    if (fileName.endsWith(".jpg")) {
      if (SPIFFS.exists("/" + fileName)) { // Final validation
        Serial.printf("Valid file found: %s\n", fileName.c_str());
        imageFiles.push_back("/" + fileName); // Keep correct path
      } else {
        Serial.printf("Warning: invalid file detected: %s\n", fileName.c_str());
      }
    }
    file = root.openNextFile();
  }

  // Trier les fichiers par ordre alphabétique
  std::sort(imageFiles.begin(), imageFiles.end());

  if (imageFiles.empty()) {
    Serial.println("No valid JPG images found in SPIFFS.");
  } else {
    Serial.printf("%d valid images found.\n", imageFiles.size());
    for (const auto& file : imageFiles) {
      Serial.println(file); // Afficher l'ordre trié
    }
  }
}


// --- Display a JPG image from SPIFFS ---
void drawImage(const char *filepath) {
  if (!SPIFFS.exists(filepath)) {
    M5.Lcd.printf("File not found: %s\n", filepath);
    return;
  }
  File f = SPIFFS.open(filepath, "r");
  if (!f) {
    M5.Lcd.printf("Error: unable to open file: %s\n", filepath);
    return;
  }

  //M5.Lcd.clear();
  Serial.printf("Displaying image: %s\n", filepath);
  M5.Display.drawJpg((Stream*)&f);
  f.close();

  Serial.println("Display done!");
}

// --- Generate the HTML page with the list of files ---
void handleListFiles() {
  String html = FPSTR(HTML_HEADER);
  html += "<h1>Available Files</h1>";
  html += "<p class='nav'><a class='btn' href='/'>Home</a><a class='btn' href='/upload'>Upload</a>";
  html += "<a class='btn danger' href='/delete-all' onclick='return confirm(\"Delete all files?\");'>Delete All</a></p>";
  html += "<ul>";

  for (const auto& file : imageFiles) {
    html += "<li>";
    html += file;
    html += " <a class='btn danger' href='/delete?file=" + file + "' onclick='return confirm(\"Delete this file?\");'>Delete</a>";
    html += "</li>";
  }

  html += "</ul>";
  html += FPSTR(HTML_FOOTER);

  server.send(200, "text/html", html);
}

// --- Handle file deletion ---
void handleDeleteFile() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "File not specified!");
    return;
  }

  String fileName = server.arg("file");
  if (SPIFFS.exists(fileName)) {
    if (SPIFFS.remove(fileName)) {
      loadImageFiles(); // Reload list after deletion
      server.send(200, "text/plain", "File successfully deleted!");
      Serial.printf("File deleted: %s\n", fileName.c_str());
    } else {
      server.send(500, "text/plain", "Error while deleting file!");
      Serial.printf("Error: unable to delete file: %s\n", fileName.c_str());
    }
  } else {
    server.send(404, "text/plain", "File not found!");
    Serial.printf("File not found for deletion: %s\n", fileName.c_str());
  }
}

void handleDeleteAllFiles() {
  bool success = true;
  for (const auto& file : imageFiles) {
    if (!SPIFFS.remove(file)) {
      success = false;
      break;
    }
  }

  loadImageFiles();
  if (success) {
    server.send(200, "text/plain", "All files successfully deleted!");
  } else {
    server.send(500, "text/plain", "Error while deleting all files!");
  }
}
  
void handleRoot() {
  String html = FPSTR(HTML_HEADER);
  html += "<h1>Home Page</h1>";
  html += "<div class='nav'>";
  html += "<a class='btn' href='/list'>View Files</a>";
  html += "<a class='btn' href='/upload'>Upload an image</a>";
  html += "<a class='btn' href='/settings'>Settings</a>";
  html += "</div>";
  html += "<p>Welcome to PixelFlow management page.</p>";
  html += FPSTR(HTML_FOOTER);

  server.send(200, "text/html", html);
}

void handleSettingsPage() {
  String html = FPSTR(HTML_HEADER);
  html += "<h1>Image Settings</h1>";
  html += "<p class='nav'><a class='btn' href='/'>Home</a>";
  html += "<form method='POST' action='/update-settings'>";
  html += "<label for='interval'>Image Change Interval (ms):</label>";
  html += "<input type='number' id='interval' name='interval' value='" + String(IMAGE_CHANGE_INTERVAL) + "' required><br><br>";
  html += "<input class='btn' type='submit' value='Save'>";
  html += "</form>";
  html += FPSTR(HTML_FOOTER);

  server.send(200, "text/html", html);
}

void handleUpdateSettings() {
  if (!server.hasArg("interval")) {
    server.send(400, "text/plain", "Invalid request!");
    return;
  }

  IMAGE_CHANGE_INTERVAL = server.arg("interval").toInt();
  saveConfig(); // Enregistrer les modifications

  String html = FPSTR(HTML_HEADER);
  html += "<h1>Settings Updated</h1>";
  html += "<p>New interval: " + String(IMAGE_CHANGE_INTERVAL) + " ms</p>";
  html += "<a class='btn' href='/settings'>Back to Settings</a>";
  html += FPSTR(HTML_FOOTER);

  server.send(200, "text/html", html);
}

// ----------------------------------------------
// Add 'multiple' to allow multiple file selection
// ----------------------------------------------
void handleUploadForm() {
  String html = FPSTR(HTML_HEADER);
  html += "<h1>Upload JPG Images</h1>";
  html += "<h3>Upload files with .jpg extension and correct resolution.</h3>";
  html += "<p class='nav'><a class='btn' href='/'>Home</a><a class='btn' href='/list'>View the list</a></p>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<label>JPG Files: <input type='file' name='data' accept='.jpg' multiple></label><br><br>";
  html += "<input class='btn' type='submit' value='Upload'>";
  html += "</form>";
  html += FPSTR(HTML_FOOTER);

  server.send(200, "text/html", html);
}

// Called after the form submission (POST)
void handleFileUploadPost() {
  server.send(200, "text/plain", "Images uploaded!");
  // Reload files after an upload
  loadImageFiles();
}

File uploadFile;

// This function is called repeatedly for each file chunk
// and for each file in the multi-upload sequence
void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  // A new file has started
  if (upload.status == UPLOAD_FILE_START) {
    String fullPath = String(JPG_FOLDER) + upload.filename;
    uploadFile = SPIFFS.open(fullPath, FILE_WRITE);
    Serial.printf("Upload Start: %s\n", upload.filename.c_str());
  }
  // Writing current chunk
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  }
  // File finished
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    Serial.printf("Upload End: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
  }
}

// ---------------------------------------------------------
// 2) WiFi and server management
// ---------------------------------------------------------
void startWebServer() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  Serial.print("AP started. IP = ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_GET, handleUploadForm);
  server.on("/upload", HTTP_POST, handleFileUploadPost, handleFileUpload);
  server.on("/list", HTTP_GET, handleListFiles);
  server.on("/delete", HTTP_GET, handleDeleteFile);
  server.on("/delete-all", HTTP_GET, handleDeleteAllFiles);
  server.on("/settings", HTTP_GET, handleSettingsPage);
  server.on("/update-settings", HTTP_POST, handleUpdateSettings);

  server.begin();
  serverRunning = true;
}

void stopWebServer() {
  server.stop();
  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);
  serverRunning = false;
}

// ---------------------------------------------------------
//   Setup / Loop
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);

  auto cfg = M5.config();
  M5.begin(cfg);

  // Afficher le message de démarrage centré
  M5.Lcd.clear(BLACK); // Efface l'écran et définit le fond en noir
  M5.Lcd.setTextColor(WHITE, BLACK); // Texte blanc sur fond noir
  M5.Lcd.setTextSize(1); // Définir la taille du texte

  // Calcul pour centrer "PixelFlow"
  int16_t centerX = (M5.Lcd.width() - M5.Lcd.textWidth("PixelFlow")) / 2;
  int16_t centerY = (M5.Lcd.height() - (M5.Lcd.fontHeight() * 2)) / 2;

  M5.Lcd.setCursor(centerX, centerY); // Positionner le texte centré horizontalement
  M5.Lcd.println("PixelFlow");

  // Calcul pour centrer "by 7h30th3r0n3" en dessous
  int16_t subTextY = centerY + M5.Lcd.fontHeight(); // Une ligne en dessous
  int16_t subTextX = (M5.Lcd.width() - M5.Lcd.textWidth("by 7h30th3r0n3")) / 2;

  M5.Lcd.setCursor(subTextX, subTextY);
  M5.Lcd.println("by 7h30th3r0n3");
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Error");
    while (true);
  }

  loadConfig(); // Charger la configuration
  loadImageFiles(); // Charger les fichiers d'images

  Serial.println("Press BtnA to start the WiFi server");
}




void loop() {
  M5.update();

  // Button A => toggle the server ON/OFF
  if (M5.BtnA.wasPressed()) {
    if (!serverRunning) {
      startWebServer();
      Serial.println("Web server ON");
    } else {
      stopWebServer();
      M5.Display.setCursor(0, 0);
      M5.Display.setTextSize(1);
      M5.Lcd.printf("WiFi Off");
      Serial.println("Web server OFF");
    }
  }

  // Handle server
  if (serverRunning) {
    server.handleClient();
    M5.Display.setCursor(0, 0);
    M5.Display.setTextSize(1);
    M5.Lcd.printf("WiFi On\nIP :192.168.4.1");
  }

  // Rotate images every xx seconds
  if (!imageFiles.empty() && millis() - lastImageChange >= IMAGE_CHANGE_INTERVAL) {
    drawImage(imageFiles[currentImageIndex].c_str());
    currentImageIndex = (currentImageIndex + 1) % imageFiles.size();
    lastImageChange = millis();
  }
}
