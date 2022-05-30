/**
 * @file usermod_nanoleaf_display.h
 * @author Projects with Red
 * @brief Controls 28 NanoLeaf segments, for a 4 digit display, made up of 7-segment digits.
 */


#pragma once

#include "wled.h"
#include <DHT.h>

#define DHTPIN 17
#define DHTTYPE DHT11


DHT dht11(DHTPIN, DHTTYPE);

const int NUM_OF_SEGMENTS   = 28;   // Number of total segments the whole display contains.
const int ADDR_LEDS_PER_SEG = 3;    // Number of addressable LEDs (ICs) per segment.
const int SEGS_PER_DIGIT    = 7;    // Number of segments that make up 1 digit.
const int NUM_DIGITS        = 10;   // Number of digits each 7 segment digit can display, 0-9.
const int NUM_FIRST_LEDS    = 3;    // Number of first set of 3 addressable LEDs which are only used to set colours.


class NanoLeafDisplay : public Usermod {
  private:

    WS2812FX::Segment *segments[NUM_OF_SEGMENTS];

    // The first three segments are used to control the colours.
    // These are the ones that will be inside the electronics housing, just used to control the colours.
    WS2812FX::Segment *firstSeg;
    WS2812FX::Segment *secondSeg;
    WS2812FX::Segment *thirdSeg;

    
    /* This array stores which segments to turn off for each digit. Where each digit is the corresponding array index.
       The tlayout of each segments is this:
       - 5 -
      6     4
       - 3 -
      2     0
       - 1 -
    */
    const int DIGITS[NUM_DIGITS][NUM_OF_SEGMENTS] = {
      {3, -1, -1, -1, -1, -1, -1},  // For the digit 0, need to only turn off middle segment, which is 3 (see layout above).
      {1, 2, 3, 5, 6, -1, -1},      // 1, -1 is added to fill the array.
      {0, 6, -1, -1, -1, -1, -1},   // 2
      {2, 6, -1, -1, -1, -1, -1},   // 3
      {1, 2, 5, -1, -1, -1, -1},    // 4
      {2, 4, -1, -1, -1, -1, -1},   // 5
      {4, -1, -1, -1, -1, -1, -1},  // 6
      {1, 2, 3, 6, -1, -1, -1},     // 7
      {-1, -1, -1, -1, -1, -1, -1}, // 8
      {2, -1, -1, -1, -1, -1, -1}   // 9
    };


    int midSegmentIndex = NUM_OF_SEGMENTS / 2;  // For the two tone mode.


    int currentHour;
    int currentMin;
    int currentSec;

    int currentTemp;
    int currentHumid;


    // Variables/options for the usermod settings page in the WLED UI.
    bool inTwoToneMode = true;
    bool inSeriesMode = false;
    bool inSecsMinsMode = false;
    bool inManualMode = false;
    bool inTempHumidMode = false;

    // For the manual mode.
    int digit0Value = 0;
    int digit1Value = 0;
    int digit2Value = 0;
    int digit3Value = 0;


    // Keeping track of last values of the modes.
    bool lastTwoToneMode = inTwoToneMode;
    bool lastInSeriesMode = inSeriesMode;
    bool lastTempHumidMode = inTempHumidMode;


    // Adding dht11 sensor delay to ensure there is a significant amount of time between each sensor reading.
    long lastDHT11TimeRead = 0;
    int dht11ReadingDelay = 10000;  // Time between each DHT11 reading in ms.
    

  public:
    void setup() {
      if(inTwoToneMode || inTempHumidMode) {
        setSegments(ADDR_LEDS_PER_SEG, NUM_OF_SEGMENTS);
        getSegments(NUM_OF_SEGMENTS);
      }

      initDHT11Sensor();
    }


    /**
     * @brief Called every time the WiFi is (re)connected.
     */
    void connected() {
      updateTime();
      readTempAndHumid();
    }


    void loop() {
      if(inTwoToneMode) {
        twoTone();
      } else if(inTempHumidMode) {
        displayTempAndHumid();
      }
    }


    /**
     * @brief This is used since strip.setPixelColor() needs to be called in this method to update.
     */
    void handleOverlayDraw() {
      // inSeriesMode is handled in the handleOverlayDraw() method, because it uses strip.setPixelColor().
      if(inSeriesMode) {
        inSeries();
      }
    }


    /**
     * @brief Handle the two tone mode, where the left 2 and right 2 digits have their own colours/effects.
     */
    void twoTone() {
      for(int i = 0; i < NUM_OF_SEGMENTS; i++) {
        if(i >= midSegmentIndex) {
          segCopyProps(*segments[i], *firstSeg, i);
        } else {
          segCopyProps(*segments[i], *secondSeg, i);
        }
      }

      displayTime();
    }


    /**
     * @brief Handle the in series mode, where all segments are considered as one long LED strip.
     */
    void inSeries() {
      displayTime();
    }


    /**
     * @brief Display the current time in the NanoLeaf display.
     */
    void displayTime() {
      if(inManualMode) {
        setDigit(0, digit0Value);
        setDigit(1, digit1Value);
        setDigit(2, digit2Value);
        setDigit(3, digit3Value);
      } else {
        updateTime();

        if(inSecsMinsMode) {
          int minDigit1 = currentMin / 10;
          int minDigit2 = currentMin % 10;
          setDigit(0, minDigit1);
          setDigit(1, minDigit2);

          int secDigit1 = currentSec / 10;
          int secDigit2 = currentSec % 10;
          setDigit(2, secDigit1);
          setDigit(3, secDigit2);
        } else {
          int hourDigit1 = currentHour / 10;
          int hourDigit2 = currentHour % 10;
          setDigit(0, hourDigit1);
          setDigit(1, hourDigit2);

          int minDigit1 = currentMin / 10;
          int minDigit2 = currentMin % 10;
          setDigit(2, minDigit1);
          setDigit(3, minDigit2);
        }
      }
    }


    /**
     * @brief Obtain the current time using WiFi through the global localTime variable and updates the corresponding variables.
     */
    void updateTime() {
      updateLocalTime();
      currentHour = hour(localTime);
      currentMin = minute(localTime);
      currentSec = second(localTime);
    }


    /**
     * @brief Create/set all the segments required.
     * 
     * @param addrLedsPerSeg number of addressable LEDs (ICs) per segment.
     * @param numOfSegments number of total segments the whole display contains.
     */
    void setSegments(int addrLedsPerSeg, int numOfSegments) {

      // For the first LEDs that are not part of the NanoLeaf. They are only used to control the colours.
      for(int i = 0; i < NUM_FIRST_LEDS; i++) {
        strip.setSegment(i, i, i + 1);
      }

      int segStart = NUM_FIRST_LEDS;
      int segEnd = addrLedsPerSeg + NUM_FIRST_LEDS;

      for(int i = 0; i < numOfSegments; i++) {
        strip.setSegment(i + NUM_FIRST_LEDS, segStart, segEnd);

        segStart += addrLedsPerSeg;
        segEnd += addrLedsPerSeg;
      }
    }


    /**
     * @brief Get all the segment objects and store them in an array.
     * 
     * @param numOfSegments number of total segments the whole display contains.
     */
    void getSegments(int numOfSegments) {
      firstSeg = &strip.getSegment(0);
      secondSeg = &strip.getSegment(1);
      thirdSeg = &strip.getSegment(2);

      for(int i = 0; i < numOfSegments; i++) {
        segments[i] = &strip.getSegment(i + NUM_FIRST_LEDS);
      }
    }


    /**
     * @brief Copy all properties of a segment to another segment.
     * 
     * @param seg1 The segment that will change.
     * @param seg2 The segment where the properties are obtained from.
     * @param seg1Id Segment 1 id which is also its index in the segments array.
     */
    void segCopyProps(WS2812FX::Segment& seg1, WS2812FX::Segment& seg2, int seg1Id) {
      seg1.setColor(0, seg2.colors[0], seg1Id + NUM_FIRST_LEDS);
      strip.setMode(seg1Id + NUM_FIRST_LEDS, seg2.mode);
      seg1.speed = seg2.speed;
      seg1.intensity = seg2.intensity;
      seg1.palette = seg2.palette;
    }


    /**
     * @brief Set the specified digit equal to a value (0-9).
     * 
     * @param digit Which digit to change, digit 0 refers to the left most digit on the display.
     * @param value What value to set the digit equal to (0-9).
     */
    void setDigit(int digit, int value) {  
      int digitIndex = NUM_OF_SEGMENTS - (SEGS_PER_DIGIT + (digit * SEGS_PER_DIGIT));

      for(int i = 0; i < NUM_DIGITS; i++) {
        int segIndex = DIGITS[value][i];

        // No more segments to turn off for that digit.
        if(segIndex == -1) break;

        if(inTwoToneMode || inTempHumidMode) {
          turnOffSeg(*segments[digitIndex + segIndex], digitIndex + segIndex);
        } else if(inSeriesMode) {
          turnOffSegPixels(digitIndex + segIndex);
        }
      }
    }


    /**
     * @brief Turn off a segment.
     * 
     * @param seg The segment to turn off.
     * @param segId The segment ID which is also its index in the segments array.
     */
    void turnOffSeg(WS2812FX::Segment& seg, int segId) {
      seg.setColor(0, 0x000000, segId + NUM_FIRST_LEDS);
      seg.setColor(1, 0x000000, segId + NUM_FIRST_LEDS);
      seg.setColor(2, 0x000000, segId + NUM_FIRST_LEDS);
      seg.setOption(0, 0);
    }


    /**
     * @brief Turn off the corresponding pixels/LEDs to turn off the whole segment, used for the in series mode.
     * 
     * @param segment The segment to turn off.
     */
    void turnOffSegPixels(int segment) {
      int startIndex = NUM_FIRST_LEDS + (segment * ADDR_LEDS_PER_SEG);

      for(int i = 0; i < ADDR_LEDS_PER_SEG; i++) {
        strip.setPixelColor(startIndex + i, 0x000000);
      }
    }


    /**
     * @brief Initialise the DHT11 sensor and take the first reading.
     */
    void initDHT11Sensor() {
      dht11.begin();
      
      currentTemp = dht11.readTemperature();
      currentHumid = dht11.readHumidity();
    }


    /**
     * @brief Display the current temperature and humidity in the NanoLeaf display.
     */
    void displayTempAndHumid() {
      readTempAndHumid();
      setTempHumidColors();

      int tempDigit1 = currentTemp / 10;      
      int tempDigit2 = currentTemp % 10;
      setDigit(0, tempDigit1);
      setDigit(1, tempDigit2);

      int humidDigit1 = currentHumid / 10;
      int humidDigit2 = currentHumid % 10;
      setDigit(2, humidDigit1);
      setDigit(3, humidDigit2);
    }


    /**
     * @brief Set the temperature and humidity colours/props using the first 3 segments.
     */
    void setTempHumidColors() {
      for(int i = 0; i < NUM_OF_SEGMENTS; i++) {
        if(i >= midSegmentIndex) {
          if(currentTemp > 0) {
            segCopyProps(*segments[i], *firstSeg, i);
          } else {  
            segCopyProps(*segments[i], *secondSeg, i);
          }
        } else {
          segCopyProps(*segments[i], *thirdSeg, i);
        }
      }
    }


    /**
     * @brief Take a reading from the DHT11 sensor for the temp and humid with a delay in-between each reading.
     */
    void readTempAndHumid() {
      if((millis() - lastDHT11TimeRead) > dht11ReadingDelay) {
        lastDHT11TimeRead = millis();

        currentTemp = dht11.readTemperature();
        currentHumid = dht11.readHumidity();
      }
    }


    /**
     * @brief Refer to the WLED docs.
     * 
     * @param root 
     */
    void readFromJsonState(JsonObject& root) {
      userVar0 = root["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
    }


    /**
     * @brief Add the usermod options to the WLED UI.
     * 
     * @param root 
     */
    void addToConfig(JsonObject& root) {
      JsonObject top = root.createNestedObject("NanoLeafDisplay");
      top["Use TwoTone Style"] = inTwoToneMode;
      top["Use InSeries Style"] = inSeriesMode;
      top["Show Secs and Mins"] = inSecsMinsMode;
      top["Manual Mode"] = inManualMode;
      top["Digit1 value"] = digit0Value;
      top["Digit2 value"] = digit1Value;
      top["Digit3 value"] = digit2Value;
      top["Digit4 value"] = digit3Value;
      top["Temp and Humid Mode"] = inTempHumidMode;
    }


    /**
     * @brief Read the usermod options from the WLED UI.
     * 
     * @param root 
     * @return true 
     * @return false 
     */
    bool readFromConfig(JsonObject& root) {
      JsonObject top = root["NanoLeafDisplay"];

      bool configComplete = !top.isNull();

      configComplete &= getJsonValue(top["Use TwoTone Style"], inTwoToneMode);
      configComplete &= getJsonValue(top["Use InSeries Style"], inSeriesMode);
      configComplete &= getJsonValue(top["Show Secs and Mins"], inSecsMinsMode);
      configComplete &= getJsonValue(top["Manual Mode"], inManualMode);
      configComplete &= getJsonValue(top["Digit1 value"], digit0Value);
      configComplete &= getJsonValue(top["Digit2 value"], digit1Value);
      configComplete &= getJsonValue(top["Digit3 value"], digit2Value);
      configComplete &= getJsonValue(top["Digit4 value"], digit3Value);
      configComplete &= getJsonValue(top["Temp and Humid Mode"], inTempHumidMode);

      if(inSeriesMode && (lastInSeriesMode != inSeriesMode)) {
        strip.resetSegments();
        strip.setSegment(0, NUM_FIRST_LEDS, (NUM_FIRST_LEDS + (NUM_OF_SEGMENTS * 3)));
      } else if((inTwoToneMode && (lastTwoToneMode != inTwoToneMode)) || 
                (inTempHumidMode && (lastTempHumidMode != inTempHumidMode))) {
        setSegments(ADDR_LEDS_PER_SEG, NUM_OF_SEGMENTS);
        getSegments(NUM_OF_SEGMENTS);
      }

      lastTwoToneMode = inTwoToneMode;
      lastInSeriesMode = inSeriesMode;
      lastTempHumidMode = inTempHumidMode;

      return configComplete;
    }

    uint16_t getId() {
      return USERMOD_ID_EXAMPLE;
    }
};