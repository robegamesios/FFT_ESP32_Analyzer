#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/********************************************************************************************************************************************************
*                                                                                                                                                       *
   Project:         FFT Spectrum Analyzer
   Target Platform: ESP32
*                                                                                                                                                       *
   Version: 1.0
   Hardware setup: See github
   Spectrum analyses done with analog chips MSGEQ7
*                                                                                                                                                       *
   Mark Donners
   The Electronic Engineer
   Website:   www.theelectronicengineer.nl
   facebook:  https://www.facebook.com/TheelectronicEngineer
   youtube:   https://www.youtube.com/channel/UCm5wy-2RoXGjG2F9wpDFF3w
   github:    https://github.com/donnersm
*                                                                                                                                                       *
*********************************************************************************************************************************************************
  Version History
   1.0 First release, code extraced from 14 band spectrum analyzer 3.00 and modified to by used with FFT on a ESP32. No need for frequency board or
       MCGEQ7 chips.
       - HUB75 interface or
       - WS2812 leds ( matrix/ledstrips)
       - 8/16/32 or 64 channel analyzer
       - calibration for White noise, pink noise, brown noise sensitivity included and selectable
       - Fire screensaver
       - Display of logo and interface text when used with HUB75
*                                                                                                                                                       *
*********************************************************************************************************************************************************
  Version FFT 1.0 release July 2021
*********************************************************************************************************************************************************
   Status   | Description
   Open     | Some Hub75 displays use a combination of chipsets of are from a different productions batch which will not work with this libary
   Open     | Sometime the long press for activating/de-activating the autoChange Pattern mode doesn't work
   Solved   | When using 64 bands, band 0 is always at max value. This was caused by the array dize [64]-> solved by chnaging it to 65
  Not a bug | Different types of HUB75 displays require different libary settings.It is what it is and it all depends on what the distributer sends you.
            | For into on the libary settings, see the library documentation on Github: https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA
  Wish      | Web interface. not possible without some heavy workaround cant use WIFI and ADC at same time
* *******************************************************************************************************************************************************
  People who inspired me to do this build and figure out how stuff works:
  Dave Plummer         https://www.youtube.com/channel/UCNzszbnvQeFzObW0ghk0Ckw
  Mrfaptastic          https://github.com/mrfaptastic
  Scott Marley         https://www.youtube.com/user/scottmarley85
  Brian Lough          https://www.youtube.com/user/witnessmenow
  atomic14             https://www.youtube.com/channel/UC4Otk-uDioJN0tg6s1QO9lw

  Make sure your arduino IDE settings: Compiler warnings is set to default to make sure the code will compile                                           */

// Wifi Manager and OTA update
#include <WiFiManager.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

const char *host = "esp32";
WiFiManager wm;
WebServer server(80);

/* Style */
String style =
    "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
    "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
    "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
    "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
    "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
    ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Server Index Page */
String serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"

    "<form name=selectPattern>"
    "<h2>Select Visualizer Pattern</h2>"
    "<p>This will change the visualizer pattern.</p>"
    "<select name='option'>"
    "<option value='12' title='Auto Change every 1 Min'>Auto Change every 1 Min</option>"
    "<option value='0' title='Boxed Bars'>Boxed Bars</option>"
    "<option value='1' title='Boxed Bars 2'>Boxed Bars 2</option>"
    "<option value='2' title='Boxed Bars 3'>Boxed Bars 3</option>"
    "<option value='3' title='Red Bars'>Red Bars</option>"
    "<option value='4' title='Color Bars'>Color Bars</option>"
    "<option value='5' title='=Twins'>Twins</option>"
    "<option value='6' title='Twins 2'>Twins 2</option>"
    "<option value='7' title='Tri Bars'>Tri Bars</option>"
    "<option value='8' title='Tri Bars 2'>Tri Bars 2</option>"
    "<option value='9' title='Center Bars'>Center Bars</option>"
    "<option value='10' title='Center Bars 2'>Center Bars 2</option>"
    "<option value='11' title='Black Bars'>Black Bars</option>"
    "</select><br><br>"
    "<input onclick=updateVisualizer() class=btn value='Update Pattern'></form>"

    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<h2>Update Firmware</h2>"
    "<p>Change to a different theme.</p>"
    "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
    "<label id='file-input' for='file'>   Choose file...</label>"
    "<input type='submit' class=btn value='Update Firmware'>"
    "<br><br>"
    "<div id='prg'></div>"
    "<br><div id='prgbar'><div id='bar'></div></div><br></form>"

    "<form name=resetForm>"
    "<h2>Reset WiFi</h2>"
    "<p>This will reset the WiFi configuration.</p>"
    "<input onclick=resetWiFi() class=btn value='Reset WiFi'></form>"

    "<script>"
    "function updateVisualizer() {"
    "var selectedPatternTitle = document.forms['selectPattern']['option'].selectedOptions[0].getAttribute('title');"
    "var selectedPatternValue = document.forms['selectPattern']['option'].value;"
    "var xhr = new XMLHttpRequest();"
    "xhr.open('GET', '/update_visualizer?pattern=' + encodeURIComponent(selectedPatternValue), true);"
    "xhr.onreadystatechange = function () {"
    "if (xhr.readyState === 4 && xhr.status === 200) {"
    "alert('Selected Visualizer Pattern updated to: ' + selectedPatternTitle);"
    "}"
    "};"
    "xhr.send();"
    "}"

    "function resetWiFi() {"
    "if (confirm('Are you sure you want to reset the WiFi? This will clear WiFi configuration.')) {"
    "window.location.href = '/reset_wifi';"
    "}"
    "}"

    "function sub(obj){"
    "var fileName = obj.value.split('\\\\');"
    "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
    "};"

    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    "$.ajax({"
    "url: '/update_firmware',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData: false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "$('#bar').css('width',Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!') "
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>" +
    style;

#define VERSION "V1.0"

#include <FastLED_NeoMatrix.h>
#include <arduinoFFT.h>
#include "I2SPLUGIN.h"
#include <math.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "FFT.h"
#include "LEDDRIVER.H"
#include "Settings.h"
#include "PatternsHUB75.h"
#include "PatternsLedstrip.h"
#include "fire.h"

int skip = true;
int ButtonOnTimer = 0;
int ButtonStarttime = 0;
int ButtonSequenceCounter = 0;
int ButtonOffTimer = 0;
int ButtonStoptime = 0;

int ButtonPressedCounter = 0;
int ButtonReleasedCounter = 0;
int ShortPressFlag = 0;
int LongPressFlag = 0;
int LongerPressFlag = 0;
boolean Next_is_new_pressed = true;
boolean Next_is_new_release = true;
int PreviousPressTime = 0;
#define up 1
#define down 0
int PeakDirection = 0;

long LastDoNothingTime = 0; // only needed for screensaver
int DemoModeMem = 0;        // to remember what mode we are in when going to demo, in order to restore it after wake up
bool AutoModeMem = false;   // same story
bool DemoFlag = false;      // we need to know if demo mode was manually selected or auto engadged.

char LCDPrintBuf[30];

// Function to update visualizer settings
void updateVisualizerSettings(int pattern)
{
  if (pattern == 12)
  {
    autoChangePatterns = true;
  }
  else
  {
    autoChangePatterns = false;
    buttonPushCounter = pattern;
  }
}

// Function to print to screen
void printToScreen(const char *buf, int y)
{
  int16_t x1, y1;
  uint16_t w, h;
  dma_display->getTextBounds(buf, 0, y, &x1, &y1, &w, &h);
  dma_display->setCursor(32 - (w / 2), y);
  dma_display->setTextColor(0xffff);
  dma_display->print(buf);
}

// Function to reset WiFi Manager settings
void resetWiFiManagerSettings()
{
  wm.resetSettings();
  ESP.restart();
}

void setup()
{
  Serial.begin(115200);

  Serial.println("Setting up Audio Input I2S");
  setupI2S();
  Serial.println("Audio input setup completed");
  delay(1000);
#ifdef Ledstrip
  SetupLEDSTRIP();
#endif

#ifdef HUB75
  SetupHUB75();
  if (kMatrixHeight > 60)
  {
    dma_display->setBrightness8(32);
  }
#endif

  printToScreen("Connect to Wifi", 20);

  // setup wifi
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  wm.autoConnect("LED_Matrix_Connect");

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("wifi not connected");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host))
  { // http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex); });

  // New route handler for resetting WiFi Manager
  server.on("/reset_wifi", HTTP_GET, []()
            {
              server.sendHeader("Connection", "close");
              server.send(200, "text/html", "WiFi Manager has been reset. Your device will restart.");
              resetWiFiManagerSettings(); // Call the reset function
            });

  // New route handler for updating Visualizer config
  server.on("/update_visualizer", HTTP_GET, []()
            {
    String patternStr = server.arg("pattern"); // Retrieve the selected pattern from query parameters
    int pattern = patternStr.toInt(); // Parse the string as an integer

    server.sendHeader("Connection", "close");
    server.send(200, "text/html", "Visualizer config has been updated");
    updateVisualizerSettings(pattern); });

  /*handling uploading firmware file */
  server.on(
      "/update_firmware", HTTP_POST, []()
      {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); },
      []()
      {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  server.begin();
}

void loop()
{
  server.handleClient();
  delay(1);

  size_t bytesRead = 0;

  // ############ Step 1: read samples from the I2S Buffer ##################
  i2s_read(I2S_PORT,
           (void *)samples,
           sizeof(samples),
           &bytesRead,     // workaround This is the actual buffer size last half will be empty but why?
           portMAX_DELAY); // no timeout

  if (bytesRead != sizeof(samples))
  {
    Serial.printf("Could only read %u bytes of %u in FillBufferI2S()\n", bytesRead, sizeof(samples));
    // return;
  }

  // ############ Step 2: compensate for Channel number and offset, safe all to vReal Array   ############
  for (uint16_t i = 0; i < ARRAYSIZE(samples); i++)
  {

    vReal[i] = offset - samples[i];
    vImag[i] = 0.0; // Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
#if PrintADCRAW
    Serial.printf("%7d,", samples[i]);
#endif

#if VisualizeAudio
    Serial.printf("%d\n", samples[i]);
#endif
  }

#if PrintADCRAW
  Serial.printf("\n");
#endif

  // ############ Step 3: Do FFT on the VReal array  ############
  //  compute FFT
  FFT.DCRemoval();
  FFT.Windowing(vReal, SAMPLEBLOCK, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLEBLOCK, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLEBLOCK);
  FFT.MajorPeak(vReal, SAMPLEBLOCK, samplingFrequency);
  for (int i = 0; i < numBands; i++)
  {
    FreqBins[i] = 0;
  }
  // ############ Step 4: Fill the frequency bins with the FFT Samples ############
  float averageSum = 0.0f;
  for (int i = 2; i < SAMPLEBLOCK / 2; i++)
  {
    averageSum += vReal[i];
    if (vReal[i] > NoiseTresshold)
    {
      int freq = BucketFrequency(i);
      int iBand = 0;
      while (iBand < numBands)
      {
        if (freq < BandCutoffTable[iBand])
          break;
        iBand++;
      }
      if (iBand > numBands)
        iBand = numBands;
      FreqBins[iBand] += vReal[i];
      //  float scaledValue = vReal[i];
      //  if (scaledValue > peak[iBand])
      //    peak[iBand] = scaledValue;
    }
  }

  // bufmd[0]=FreqBins[12];
#if PrintRAWBins
  for (int y = 0; y < numBands; y++)
  {
    Serial.printf("%7.1f,", FreqBins[y]);
  }
  Serial.printf("\n");
#endif

  // ############ Step 5: Determine the VU value  and mingle in the readout...( cheating the bands ) ############ Step
  float t = averageSum / (SAMPLEBLOCK / 2);
  gVU = max(t, (oldVU * 3 + t) / 4);
  oldVU = gVU;
  if (gVU > DemoTreshold)
    LastDoNothingTime = millis(); // if there is signal in any off the bands[>2] then no demo mode

  // Serial.printf("gVu: %d\n",(int) gVU);

  for (int j = 0; j < numBands; j++)
  {
    if (CalibrationType == 1)
      FreqBins[j] *= BandCalibration_Pink[j];
    else if (CalibrationType == 2)
      FreqBins[j] *= BandCalibration_White[j];
    else if (CalibrationType == 3)
      FreqBins[j] *= BandCalibration_Brown[j];
  }

  //*
  // ############ Step 6: Averaging and making it all fit on screen
  // for (int i = 0; i < numBands; i++) {
  // Serial.printf ("Chan[%d]:%d",i,(int)FreqBins[i]);
  // FreqBins[i] = powf(FreqBins[i], gLogScale); // in case we want log scale..i leave it in here as reminder
  //  Serial.printf( " - log: %d \n",(int)FreqBins[i]);
  // }
  static float lastAllBandsPeak = 0.0f;
  float allBandsPeak = 0;
  // bufmd[1]=FreqBins[13];
  // bufmd[2]=FreqBins[1];
  for (int i = 0; i < numBands; i++)
  {
    // allBandsPeak = max (allBandsPeak, FreqBins[i]);
    if (FreqBins[i] > allBandsPeak)
    {
      allBandsPeak = FreqBins[i];
    }
  }
  if (allBandsPeak < 1)
    allBandsPeak = 1;
  //  The followinf picks allBandsPeak if it's gone up.  If it's gone down, it "averages" it by faking a running average of GAIN_DAMPEN past peaks
  allBandsPeak = max(allBandsPeak, ((lastAllBandsPeak * (GAIN_DAMPEN - 1)) + allBandsPeak) / GAIN_DAMPEN); // Dampen rate of change a little bit on way down
  lastAllBandsPeak = allBandsPeak;

  if (allBandsPeak < 80000)
    allBandsPeak = 80000;
  for (int i = 0; i < numBands; i++)
  {
    FreqBins[i] /= (allBandsPeak * 1.0f);
  }

  // Process the FFT data into bar heights
  for (int band = 0; band < numBands; band++)
  {
    int barHeight = FreqBins[band] * kMatrixHeight - 1; //(AMPLITUDE);
    if (barHeight > TOP - 2)
      barHeight = TOP - 2;

    // Small amount of averaging between frames
    barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2;

    // Move peak up
    if (barHeight > peak[band])
    {
      peak[band] = min(TOP, barHeight);
      PeakFlag[band] = 1;
    }
    bndcounter[band] += barHeight; // ten behoeve calibratie

    // if there hasn't been much of a input signal for a longer time ( see settings ) go to demo mode
    if ((millis() - LastDoNothingTime) > DemoAfterSec && DemoFlag == false)
    {
      dbgprint("In loop 1:  %d", millis() - LastDoNothingTime);
      DemoFlag = true;
      // first store current mode so we can go back to it after wake up
      DemoModeMem = buttonPushCounter;
      AutoModeMem = autoChangePatterns;
      autoChangePatterns = false;
      buttonPushCounter = 12;
#ifdef HUB75
      dma_display->clearScreen();
#endif
      dbgprint("Automode is turned of because of demo");
    }
    // Wait,signal is back? then wakeup!
    else if (DemoFlag == true && (millis() - LastDoNothingTime) < DemoAfterSec)
    { //("In loop 2:  %d", millis() - LastDoNothingTime);
      // while in demo the democounter was reset due to signal on one of the bars.
      // So we need to exit demo mode.
#ifdef HUB75
      dma_display->clearScreen();
#endif
      buttonPushCounter = DemoModeMem; // restore settings
      dbgprint("automode setting restored to: %d", AutoModeMem);
      autoChangePatterns = AutoModeMem; // restore settings
      DemoFlag = false;
    }
#if BottomRowAlwaysOn
    if (barHeight == 0)
      barHeight = 1; // make sure there is always one bar that lights up
#endif
    // Now visualize those bar heights
    switch (buttonPushCounter)
    {
    case 0:
#ifdef HUB75
      PeakDirection = down;
      BoxedBars(band, barHeight);
      BluePeak(band);
#endif
#ifdef Ledstrip
      changingBarsLS(band, barHeight);
#endif
      break;

    case 1:
#ifdef HUB75
      PeakDirection = down;
      BoxedBars2(band, barHeight);
      BluePeak(band);
#endif
#ifdef Ledstrip
      TriBarLS(band, barHeight);
      TriPeakLS(band);
#endif
      break;
    case 2:
#ifdef HUB75
      PeakDirection = down;
      BoxedBars3(band, barHeight);
      RedPeak(band);
#endif
#ifdef Ledstrip
      rainbowBarsLS(band, barHeight);
      NormalPeakLS(band, PeakColor1);
#endif
      break;
    case 3:
#ifdef HUB75
      PeakDirection = down;
      RedBars(band, barHeight);
      BluePeak(band);
#endif
#ifdef Ledstrip
      purpleBarsLS(band, barHeight);
      NormalPeakLS(band, PeakColor2);
#endif
      break;
    case 4:
#ifdef HUB75
      PeakDirection = down;
      ColorBars(band, barHeight);
#endif
#ifdef Ledstrip
      SameBarLS(band, barHeight);
      NormalPeakLS(band, PeakColor3);
#endif
      break;
    case 5:
#ifdef HUB75
      PeakDirection = down;
      Twins(band, barHeight);
      WhitePeak(band);
#endif
#ifdef Ledstrip
      SameBar2LS(band, barHeight);
      NormalPeakLS(band, PeakColor3);
#endif
      break;
    case 6:
#ifdef HUB75
      PeakDirection = down;
      Twins2(band, barHeight);
      WhitePeak(band);
#endif
#ifdef Ledstrip
      centerBarsLS(band, barHeight);
#endif
      break;
    case 7:
#ifdef HUB75
      PeakDirection = down;
      TriBars(band, barHeight);
      TriPeak(band);
#endif
#ifdef Ledstrip
      centerBars2LS(band, barHeight);
#endif
      break;
    case 8:
#ifdef HUB75
      PeakDirection = up;
      TriBars(band, barHeight);
      TriPeak(band);
#endif
#ifdef Ledstrip
      centerBars3LS(band, barHeight);
#endif
      break;
    case 9:
#ifdef HUB75
      PeakDirection = down;
      centerBars(band, barHeight);
#endif
#ifdef Ledstrip
      BlackBarLS(band, barHeight);
      outrunPeakLS(band);
#endif
      break;
    case 10:
#ifdef HUB75
      PeakDirection = down;
      centerBars2(band, barHeight);
#endif
#ifdef Ledstrip
      BlackBarLS(band, barHeight);
      NormalPeakLS(band, PeakColor5);
#endif
      break;
    case 11:
#ifdef HUB75
      PeakDirection = down;
      BlackBars(band, barHeight);
      DoublePeak(band);
#endif
#ifdef Ledstrip
      BlackBarLS(band, barHeight);
      TriPeak2LS(band);
#endif
      break;
    case 12:
#ifdef HUB75
      make_fire(); // go to demo mode
#endif
#ifdef Ledstrip
      matrix->fillRect(0, 0, matrix->width(), 1, 0x0000); // delete the VU meter
      make_fire();
#endif
      break;
    }

    // Save oldBarHeights for averaging later
    oldBarHeights[band] = barHeight;
  }
  // for calibration
  // bndcounter[h]+=barHeight;
  if (loopcounter == 256)
  {
    loopcounter = 0;
#if CalibratieLog
    Calibration();
    for (int g = 0; g < numBands; g++)
      bndcounter[g] = 0;
#endif
  }
  loopcounter++;

  if (buttonPushCounter != 12)
    DrawVUMeter(0); // Draw it when not in screensaver mode

#if PrintRAWBins
  Serial.printf("\n");
  // delay(10);
#endif

  // Decay peak
  EVERY_N_MILLISECONDS(Fallingspeed)
  {
    for (byte band = 0; band < numBands; band++)
    {
      if (PeakFlag[band] == 1)
      {
        PeakTimer[band]++;
        if (PeakTimer[band] > Peakdelay)
        {
          PeakTimer[band] = 0;
          PeakFlag[band] = 0;
        }
      }
      else if ((peak[band] > 0) && (PeakDirection == up))
      {
        peak[band] += 1;
        if (peak[band] > (kMatrixHeight + 10))
          peak[band] = 0;
      } // when to far off screen then reset peak height
      else if ((peak[band] > 0) && (PeakDirection == down))
      {
        peak[band] -= 1;
      }
    }
    colorTimer++;
  }

  EVERY_N_MILLISECONDS(10)
  colorTimer++; // Used in some of the patterns

  EVERY_N_SECONDS(secToChangePattern)
  {
    // if (FastLED.getBrightness() == 0) FastLED.setBrightness(BRIGHTNESSMARK);  //Re-enable if lights are "off"
    if (autoChangePatterns)
    {
      buttonPushCounter = (buttonPushCounter + 1) % 12;
#ifdef HUB75
      dma_display->clearScreen();
#endif
    }
  }

#ifdef Ledstrip
  delay(1); // needed to give fastled a minimum recovery time
  FastLED.show();
#endif

} // loop end

// BucketFrequency
//
// Return the frequency corresponding to the Nth sample bucket.  Skips the first two
// buckets which are overall amplitude and something else.

int BucketFrequency(int iBucket)
{
  if (iBucket <= 1)
    return 0;
  int iOffset = iBucket - 2;
  return iOffset * (samplingFrequency / 2) / (SAMPLEBLOCK / 2);
}

void DrawVUPixels(int i, int yVU, int fadeBy = 0)
{
  CRGB VUC;
  if (i > (PANE_WIDTH / 3))
  {
    VUC.r = 255;
    VUC.g = 0;
    VUC.b = 0;
  }
  else if (i > (PANE_WIDTH / 5))
  {
    VUC.r = 255;
    VUC.g = 255;
    VUC.b = 0;
  }
  else
  { // green
    VUC.r = 0;
    VUC.g = 255;
    VUC.b = 0;
  }

#ifdef Ledstrip
  int xHalf = matrix->width() / 2;
  //  matrix->drawPixel(xHalf-i-1, yVU, CRGB(0,100,0).fadeToBlackBy(fadeBy));
  //  matrix->drawPixel(xHalf+i,   yVU, CRGB(0,100,0).fadeToBlackBy(fadeBy));
  matrix->drawPixel(xHalf - i - 1, yVU, CRGB(VUC.r, VUC.g, VUC.b).fadeToBlackBy(fadeBy));
  matrix->drawPixel(xHalf + i, yVU, CRGB(VUC.r, VUC.g, VUC.b).fadeToBlackBy(fadeBy));

#endif

#ifdef HUB75
  int xHalf = PANE_WIDTH / 2;
  dma_display->drawPixelRGB888(xHalf - i - 2, yVU, VUC.r, VUC.g, VUC.b);     // left side of screen line 0
  dma_display->drawPixelRGB888(xHalf - i - 2, yVU + 1, VUC.r, VUC.g, VUC.b); // left side of screen line 1
  dma_display->drawPixelRGB888(xHalf + i + 1, yVU, VUC.r, VUC.g, VUC.b);     // right side of screen line 0
  dma_display->drawPixelRGB888(xHalf + i + 1, yVU + 1, VUC.r, VUC.g, VUC.b); // right side of screen line 1
#endif
}

void DrawVUMeter(int yVU)
{
  static int iPeakVUy = 0;           // size (in LED pixels) of the VU peak
  static unsigned long msPeakVU = 0; // timestamp in ms when that peak happened so we know how old it is
  const int MAX_FADE = 256;
#ifdef HUB75
  for (int x = 0; x < PANE_WIDTH; x++)
  {
    dma_display->drawPixelRGB888(x, yVU, 0, 0, 0);
    dma_display->drawPixelRGB888(x, yVU + 1, 0, 0, 0);
  }
#endif
#ifdef Ledstrip
  matrix->fillRect(0, yVU, matrix->width(), 1, 0x0000);
#endif
  if (iPeakVUy > 1)
  {
    int fade = MAX_FADE * (millis() - msPeakVU) / (float)1000;
    DrawVUPixels(iPeakVUy, yVU, fade);
  }
  int xHalf = (PANE_WIDTH / 2) - 1;
  int bars = map(gVU, 0, MAX_VU, 1, xHalf);
  bars = min(bars, xHalf);
  if (bars > iPeakVUy)
  {
    msPeakVU = millis();
    iPeakVUy = bars;
  }
  else if (millis() - msPeakVU > 1000)
    iPeakVUy = 0;
  for (int i = 0; i < bars; i++)
    DrawVUPixels(i, yVU);
}

void Calibration(void)
{
  Serial.printf("BandCalibration_XXXX[%1d]=\n{", numBands);
  long Totalbnd = 0;

  for (int g = 0; g < numBands; g++)
  {
    if (bndcounter[g] > Totalbnd)
      Totalbnd = bndcounter[g];
  }

  for (int g = 0; g < numBands; g++)
  {
    bndcounter[g] = Totalbnd / bndcounter[g];
    Serial.printf(" %2.2f", bndcounter[g]);
    if (g < numBands - 1)
      Serial.printf(",");
    else
      Serial.print(" };\n");
  }
}

//**************************************************************************************************
//                                          D B G P R I N T                                        *
//**************************************************************************************************
// Send a line of info to serial output.  Works like vsprintf(), but checks the DEBUG flag.        *
// Print only if DEBUG flag is true.  Always returns the formatted string.                         *
// Usage dbgprint("this is the text you want: %d", variable);
//**************************************************************************************************
void dbgprint(const char *format, ...)
{
  if (DEBUG)
  {
    static char sbuf[DEBUG_BUFFER_SIZE];            // For debug lines
    va_list varArgs;                                // For variable number of params
    va_start(varArgs, format);                      // Prepare parameters
    vsnprintf(sbuf, sizeof(sbuf), format, varArgs); // Format the message
    va_end(varArgs);                                // End of using parameters
    if (DEBUG)                                      // DEBUG on?
    {
      Serial.print("Debug: "); // Yes, print prefix
      Serial.println(sbuf);    // and the info
    }
    // return sbuf; // Return stored string
  }
}

void make_fire()
{
  uint16_t i, j;

  if (t > millis())
    return;
  t = millis() + (1000 / FPS);

  // First, move all existing heat points up the display and fade

  for (i = rows - 1; i > 0; --i)
  {
    for (j = 0; j < cols; ++j)
    {
      uint8_t n = 0;
      if (pix[i - 1][j] > 0)
        n = pix[i - 1][j] - 1;
      pix[i][j] = n;
    }
  }

  // Heat the bottom row
  for (j = 0; j < cols; ++j)
  {
    i = pix[0][j];
    if (i > 0)
    {
      pix[0][j] = random(NCOLORS - 6, NCOLORS - 2);
    }
  }

  // flare
  for (i = 0; i < nflare; ++i)
  {
    int x = flare[i] & 0xff;
    int y = (flare[i] >> 8) & 0xff;
    int z = (flare[i] >> 16) & 0xff;
    glow(x, y, z);
    if (z > 1)
    {
      flare[i] = (flare[i] & 0xffff) | ((z - 1) << 16);
    }
    else
    {
      // This flare is out
      for (int j = i + 1; j < nflare; ++j)
      {
        flare[j - 1] = flare[j];
      }
      --nflare;
    }
  }
  newflare();

  // Set and draw
  for (i = 0; i < rows; ++i)
  {
    for (j = 0; j < cols; ++j)
    {
      // matrix -> drawPixel(j, rows - i, colors[pix[i][j]]);
      CRGB COlsplit = colors[pix[i][j]];
#ifdef HUB75
      dma_display->drawPixelRGB888(j, rows - i, COlsplit.r, COlsplit.g, COlsplit.b);
#endif
#ifdef Ledstrip
      matrix->drawPixel(j, rows - i, colors[pix[i][j]]);
#endif
    }
  }
}

void DisplayPrint(char *text)
{
#ifdef HUB75
  dma_display->fillRect(8, 8, kMatrixWidth - 16, 11, dma_display->color444(0, 0, 0));
  dma_display->setTextSize(1);
  dma_display->setTextWrap(false);
  dma_display->setCursor(10, 10);
  dma_display->print(text);
  delay(1000);
  dma_display->fillRect(8, 8, kMatrixWidth - 16, 11, dma_display->color444(0, 0, 0));
#endif
}
