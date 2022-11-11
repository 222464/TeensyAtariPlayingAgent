#include "Hierarchy.h"
#include "ImageEncoder.h"

#include <SPI.h>
#include <SD.h>

using namespace aon;

const char* fileNameH = "pong.ohr";
const char* fileNameEnc = "pong.oenc";

ByteBuffer img(32 * 32);

Hierarchy h;
Array<const IntBuffer*> inputs(2);

ImageEncoder enc;
Array<const ByteBuffer*> imgs(1);

class SDReader : public StreamReader {
public:
    File* f;

    SDReader(
        File* f
    ) {
        this->f = f;
    }

    void read(
        void* data,
        int len
    ) override {
        f->read((char*)data, len);
    }
};

// Subclass Streamwriter to make an SD writer
class SDWriter : public StreamWriter {
public:
    File* f;

    SDWriter(
        File* f
    ) {
        this->f = f;
    }

    void write(
        const void* data,
        int len
    ) override {
        f->write((const char*)data, len);
    }
};

void saveHierarchy() {
    // Remove existing file
    if (SD.exists(fileNameH))
        SD.remove(fileNameH);

    File f = SD.open(fileNameH, FILE_WRITE);

    SDWriter writer(&f);

    h.write(writer);

    f.close();
}

void loadHierarchy() {
    File f = SD.open(fileNameH, FILE_READ);

    Serial.print("Hierarchy available: ");
    Serial.println(f.available());

    SDReader reader(&f);

    // Read magic
    int magic;
    f.read((char*)&magic, sizeof(int));

    h.read(reader);

    f.close();
}

void saveImageEncoder() {
    // Remove existing file
    if (SD.exists(fileNameEnc))
        SD.remove(fileNameEnc);

    File f = SD.open(fileNameEnc, FILE_WRITE);

    SDWriter writer(&f);

    enc.write(writer);

    f.close();
}

void loadImageEncoder() {
    File f = SD.open(fileNameEnc, FILE_READ);

    Serial.print("Image encoder available: ");
    Serial.println(f.available());

    SDReader reader(&f);

    // Read magic
    int magic;
    f.read((char*)&magic, sizeof(int));

    enc.read(reader);

    f.close();
}

void setup() {
    Serial.begin(9600);

    delay(3000);

    if (!SD.begin(BUILTIN_SDCARD))
        Serial.println("Could not open SD card!");

    Serial.println("Loading...");

    // If a save file exists, load that
    if (SD.exists(fileNameH)) {
        loadHierarchy();

        Serial.println("Loaded hierarchy.");
    }
    else
        Serial.println("Could not load hierarchy!");

    if (SD.exists(fileNameEnc)) {
        loadImageEncoder();

        Serial.println("Loaded image encoder.");
    }
    else
        Serial.println("Could not load image encoder!");

    Serial.println("Loaded.");

    inputs[0] = &enc.getHiddenCIs();
    inputs[1] = &h.getPredictionCIs(1);

    imgs[0] = &img;

    Serial.println("Ready, you may now start pong.py after closing this monitor!");
}

void loop() {
    while (Serial.available() < img.size() + 4)
        delay(1);

    for (int i = 0; i < img.size(); i++)
        img[i] = Serial.read();

    float reward;

    Serial.readBytes((char*)&reward, sizeof(float));

    enc.step(imgs, true);

    h.step(inputs, true, reward);

    int action = h.getPredictionCIs(1)[0];

    Serial.write((byte)action);

    // Send reward back for checking if aligned properly
    Serial.write((char*)&reward, sizeof(float));
}
