## Audio Sample Preparation

To upload an audio sample to Arduino, you'll need to convert it to a compatible format.

### Requirements
- **Audacity** (audio editor)
- **wav2c** converter tool

### Step 1: Audio Conversion in Audacity

Export your audio sample with the following settings:
- **Sample Rate:** 8 kHz (8000 Hz)
- **Bit Depth:** 8-bit
- **Channels:** Mono
- **Export Format:** WAV

### Step 2: Generate Audio Data Array with wav2c

Use the [wav2c](https://github.com/olleolleolle/wav2c) tool to convert the WAV file to an Arduino-compatible array.

**Build the tool:**
```bash
make
```

**Convert audio file:**
```bash
./wav2c <input_sound_file> <output_text_file> <array_name>
```

**Example:**
```bash
./wav2c audio-sample.wav output.h sounddata
```
