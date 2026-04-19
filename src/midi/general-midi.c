//#pragma GCC optimize("Ofast")
//#pragma once

#include "config.h"  
#include "inttypes.h"
#include <ctype.h>

#include <math.h>
#include "general-midi.h"


#define MAX_MIDI_VOICES 32
#define MIDI_CHANNELS 16

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#define SOUND_FREQUENCY (37500) //(44100)
#define RELEASE_DURATION (SOUND_FREQUENCY / 8) // Duration for note release

typedef struct midi_voice_s {
    uint8_t voice_slot;
    // uint8_t playing;
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
    uint8_t velocity_base;

    uint8_t *sample;
    int32_t frequency_m100;
    uint16_t sample_position;
    uint16_t release_position;
} midi_voice_t;

typedef struct midi_channel_s {
    uint8_t program;
    uint8_t volume;
    int pitch;//int32_t pitch;
} midi_channel_t;

typedef struct __attribute__((packed)) {
    uint8_t command;
    uint8_t note;
    uint8_t velocity;
    uint8_t other;
} midi_command_t;

static midi_voice_t midi_voices[MAX_MIDI_VOICES] = {0};
static midi_channel_t midi_channels[MIDI_CHANNELS] = {0};

// Bitmask for active voices
static uint32_t active_voice_bitmask = 0;
// Bitmask for sustained channels
static uint32_t channels_sustain_bitmask = 0;

// Drum synthesis parameters
#define DRUM_KICK_BASE     35  // MIDI note for kick drum
#define DRUM_SNARE_BASE    38  // MIDI note for snare drum
#define DRUM_HIHAT_CLOSED  42  // MIDI note for closed hi-hat
#define DRUM_HIHAT_OPEN    46  // MIDI note for open hi-hat
#define DRUM_CRASH         49  // MIDI note for crash cymbal
#define DRUM_RIDE          51  // MIDI note for ride cymbal

// Best seeds for different percussion characteristics
static uint32_t noise_seed = 0x7EC80000;  // Best overall for drums

// Alternative excellent seeds:
// 0x7EC80000 - Optimal: Maximum length sequence, good spectral distribution
// 0xDEADBEEF - Classic: Good randomness, widely used
// 0x8B8B8B8B - Balanced: Equal bit distribution
// 0xAAAA5555 - Alternating: Good for metallic sounds
// 0x13579BDF - Fibonacci-like: Natural sounding randomness

#define SET_ACTIVE_VOICE(idx) (active_voice_bitmask |= (1U << (idx)))
#define CLEAR_ACTIVE_VOICE(idx) (active_voice_bitmask &= ~(1U << (idx)))
#define IS_ACTIVE_VOICE(idx) ((active_voice_bitmask & (1U << (idx))) != 0)

#define SET_CHANNEL_SUSTAIN(idx) (channels_sustain_bitmask |= (1U << (idx)))
#define CLEAR_CHANNEL_SUSTAIN(idx) (channels_sustain_bitmask &= ~(1U << (idx)))
#define IS_CHANNEL_SUSTAIN(idx) ((channels_sustain_bitmask & (1U << (idx))) != 0)


#define SIN_STEP (SOUND_FREQUENCY * 100 / 4096)
static  int32_t fast (sine_lookup)(const uint32_t angle) {
    const uint16_t index = (angle / SIN_STEP) & 4095; // TODO: Should it be & 4095 or % 4096???
    return index < 2048
               ? sin_m128[index < 1024 ? index : 2047 - index]
               : -sin_m128[index < 3072 ? index - 2048 : 4095 - index];
}

static  int16_t fast (generate_noise)() {
    // Linear feedback shift register for white noise
    noise_seed = (noise_seed >> 1) ^ (-(noise_seed & 1) & 0xD0000001);
    return (int16_t)((noise_seed & 0xFFFF) - 0x8000) >> 8;
}

// Drum-specific synthesis
static  int16_t fast(generate_drum_sample)(midi_voice_t* voice, uint16_t sample_position) {
    int16_t sample = 0;
    uint8_t note = voice->note;

    // Envelope calculation - drums need faster decay
    int32_t envelope = voice->velocity;

    if (note >= DRUM_KICK_BASE && note <= DRUM_KICK_BASE + 2) {
        // KICK DRUM: Low frequency sine + pitch sweep + click
        // Pitch sweep from higher to lower frequency
        int32_t sweep_freq = voice->frequency_m100 * (100 + (200 >> (sample_position >> 7))) / 100;

        // Sine component for body
        int32_t sine_val = sine_lookup(__fast_mul(sweep_freq, sample_position));

        // Add click at the beginning
        if (sample_position < 64) {
            sample = __fast_mul(envelope * 2, generate_noise()) >> 8;
        }

        // Fast exponential decay
        envelope = envelope * (65536 - (sample_position << 4)) >> 16;
        if (envelope < 0) envelope = 0;

        sample += __fast_mul(envelope, sine_val) >> 1;

    } else if (note >= DRUM_SNARE_BASE && note <= DRUM_SNARE_BASE + 2) {
        // SNARE DRUM: Mixed noise and tone
        // Tonal component around 200Hz
        int32_t tone_freq = 20000; // 200Hz at 100x scale
        int32_t sine_val = sine_lookup(__fast_mul(tone_freq, sample_position));

        // Mix noise and tone
        int16_t noise = generate_noise();

        // Sharp attack, quick decay
        if (sample_position < 100) {
            envelope = envelope;
        } else {
            envelope = envelope * (65536 - ((sample_position - 100) << 7)) >> 16;
        }
        if (envelope < 0) envelope = 0;

        // 70% noise, 30% tone for snare
        sample = __fast_mul(envelope, (noise * 7 + sine_val * 3) / 10);

    } else if (note == DRUM_HIHAT_CLOSED) {
        // CLOSED HI-HAT: High-frequency filtered noise
        int16_t noise = generate_noise();

        // High-pass filter simulation (emphasize high frequencies)
        static int16_t prev_noise = 0;
        int16_t filtered = noise - prev_noise;
        prev_noise = noise;

        // Very fast decay
        envelope = envelope * (65536 - (sample_position << 9)) >> 16;
        if (envelope < 0) envelope = 0;

        sample = __fast_mul(envelope, filtered);

    } else if (note == DRUM_HIHAT_OPEN) {
        // OPEN HI-HAT: Longer decay high-frequency noise
        int16_t noise = generate_noise();

        // High-pass filter
        static int16_t prev_noise_open = 0;
        int16_t filtered = noise - prev_noise_open;
        prev_noise_open = noise;

        // Slower decay than closed hi-hat
        envelope = envelope * (65536 - (sample_position << 6)) >> 16;
        if (envelope < 0) envelope = 0;

        sample = __fast_mul(envelope, filtered);

    } else if (note >= DRUM_CRASH && note <= DRUM_CRASH + 2) {
        // CRASH CYMBAL: Complex noise with slow decay
        int16_t noise1 = generate_noise();
        int16_t noise2 = generate_noise();

        // Mix multiple noise sources for complexity
        int16_t mixed = (noise1 + noise2) >> 1;

        // Ring modulation for metallic sound
        int32_t mod_freq = 80000; // 800Hz modulator
        int32_t modulator = sine_lookup(__fast_mul(mod_freq, sample_position));
        mixed = __fast_mul(mixed, modulator) >> 7;

        // Slow decay
        envelope = envelope * (65536 - (sample_position << 4)) >> 16;
        if (envelope < 0) envelope = 0;

        sample = __fast_mul(envelope, mixed);

    } else if (note >= DRUM_RIDE && note <= DRUM_RIDE + 2) {
        // RIDE CYMBAL: Tonal component + noise
        int32_t tone_freq = 50000; // 500Hz bell tone
        int32_t sine_val = sine_lookup(__fast_mul(tone_freq, sample_position));
        int16_t noise = generate_noise();

        // Mix tone and noise
        int16_t mixed = (sine_val + noise) >> 1;

        // Medium decay
        envelope = envelope * (65536 - (sample_position << 5)) >> 16;
        if (envelope < 0) envelope = 0;

        sample = __fast_mul(envelope, mixed);

    } else {
        // DEFAULT PERCUSSION: Generic noise burst
        int16_t noise = generate_noise();

        // Fast decay
        envelope = envelope * (65536 - (sample_position << 7)) >> 16;
        if (envelope < 0) envelope = 0;

        sample = __fast_mul(envelope, noise);
    }

    return (sample>>2);// >>2 ; // >>2 Scale down to prevent clipping
    
}

// Modified midi_sample function
int32_t fast(midi_sample)() {
    if (__builtin_expect(!active_voice_bitmask, 0)) return 0;

    int32_t sample = 0;
    uint32_t active_voices = active_voice_bitmask;

    do {
        const uint32_t voice_index = __builtin_ctz(active_voices);
        const uint32_t voice_bit = 1U << voice_index;
        active_voices ^= voice_bit;

        midi_voice_t * __restrict voice = &midi_voices[voice_index];
        const uint16_t sample_position = voice->sample_position++;

        // Check if this is a drum channel (channel 9)
        if (voice->channel == 9) {
            // Use drum synthesis
            int32_t drum_sample = generate_drum_sample(voice, sample_position);
            sample += drum_sample;

            // Drums have finite duration based on their envelope
            if (sample_position > SOUND_FREQUENCY) { // Max 1 second
                active_voice_bitmask &= ~voice_bit;
            }
        } else {
            // Original sine-based synthesis for melodic instruments
            if (__builtin_expect(sample_position == (SOUND_FREQUENCY >> 1), 0)) {
                voice->velocity -= voice->velocity >> 2;
            } else if (__builtin_expect(sample_position && sample_position == voice->release_position, 0)) {
                active_voice_bitmask &= ~voice_bit;
                continue;
            }

            const int32_t sine_val = sine_lookup(__fast_mul(voice->frequency_m100, sample_position));
            sample += __fast_mul(voice->velocity, sine_val);
        }

    } while (active_voices);

   return (sample>>2); // >>2
  //return (int16_t)(sample>>2 ) -32768;// >>2 - 32768

}

// Optimized pitch bend calculation with lookup table or approximation
static  int32_t fast(apply_pitch)(const int32_t base_frequency, const int32_t cents) {
    // Optimized: avoid division if cents is zero
    return __builtin_expect(cents == 0, 1) ? base_frequency :
           (base_frequency * cents + 5000) / 10000;
}


static  void fast(parse_midi)(const midi_command_t *message) {
    const uint8_t channel = message->command & 0xf;

    switch (message->command >> 4) {
        case 0x9: // Note ON
            if (__builtin_expect(message->velocity != 0, 1)) {
                // Поиск свободного голосового слота, используя битовые операции
                const uint32_t free_voices = ~active_voice_bitmask;
                if (__builtin_expect(free_voices != 0, 1)) {
                    const uint32_t voice_slot = __builtin_ctz(free_voices);
                    if (voice_slot < MAX_MIDI_VOICES) {
                        midi_voice_t * __restrict voice = &midi_voices[voice_slot];

                        // Инициализация голосовых данных в оптимальном порядке
                        voice->voice_slot = voice_slot;
                        voice->sample_position = 0;
                        voice->release_position = 0;
                        voice->channel = channel;
                        voice->note = message->note;
                        voice->velocity_base = message->velocity;

                        // применение тона, огибающей и громкость за один раз
                        voice->frequency_m100 = apply_pitch(
                            note_frequencies_m_100[message->note],
                            midi_channels[channel].pitch
                        );

                        const uint8_t ch_volume = midi_channels[channel].volume;
                        /*if (channel == 9) {
                            // Boost velocity for drums to make them punchier
                            voice->velocity = MIN(127, (message->velocity * 3) >> 1);
                        } else*/ {
                            voice->velocity = __builtin_expect(ch_volume != 0, 1) ?
                                (ch_volume * message->velocity) >> 7 : message->velocity;
                        }
                        SET_ACTIVE_VOICE(voice_slot);
                        break;
                    }
                }
            } // else do note off
        case 0x8: // Note OFF
            /* Probably we should
             * Find the first and last entry in the voices list with matching channel, key and look up the smallest play position
             */
            if (!IS_CHANNEL_SUSTAIN(channel)) {
                // Optimized voice search with early termination
                uint32_t voices_to_check = active_voice_bitmask;
                while (voices_to_check) {
                    const uint32_t voice_slot = __builtin_ctz(voices_to_check);
                    voices_to_check &= ~(1U << voice_slot);

                    const midi_voice_t * __restrict voice = &midi_voices[voice_slot];
                    if (voice->channel == channel && voice->note == message->note) {
                        if (__builtin_expect(channel == 9, 0)) { // Drum channel
                            CLEAR_ACTIVE_VOICE(voice_slot);
                        } else {
                            midi_voices[voice_slot].velocity >>= 1; // Bit shift for /2
                            midi_voices[voice_slot].release_position =
                                midi_voices[voice_slot].sample_position + RELEASE_DURATION;
                        }
                        break;
                    }
                }
            }
            break;


        case 0xB: // Controller Change
            switch (message->note) {
                case 0x7: // Volume change
                  //  debug_log("[MIDI] Channel %i volume %i\n", channel, midi_channels[channel].volume);
                    midi_channels[channel].volume = message->velocity;
                    for (int voice_slot = 0; voice_slot < MAX_MIDI_VOICES; ++voice_slot) {
                        if (midi_voices[voice_slot].channel == channel) {
                            midi_voices[voice_slot].velocity =
                                    message->velocity * midi_voices[voice_slot].velocity_base >> 7;
                        }
                    }
                    break;
                /*
                case 0x0A: //  Left-right pan
                    break;
                */
                case 0x40: // Sustain
                    if (message->velocity & 64) {
                        SET_CHANNEL_SUSTAIN(channel);
                    } else {
                        CLEAR_CHANNEL_SUSTAIN(channel);

                        for (int voice_slot = 0; voice_slot < MAX_MIDI_VOICES; ++voice_slot)
                            if (midi_voices[voice_slot].channel == channel) {
                                if (channel == 9) {
                                    CLEAR_ACTIVE_VOICE(voice_slot);
                                } else {
                                    midi_voices[voice_slot].velocity /= 2;
                                    midi_voices[voice_slot].release_position =
                                            midi_voices[voice_slot].sample_position + RELEASE_DURATION;
                                }
                            }
                    }
                 //   debug_log("[MIDI] Channel %i sustain %i\n", channel, message->velocity);

                    break;
                case 0x78: // All Sound Off
                case 0x7b: // All Notes Off
                    active_voice_bitmask = 0;
                    channels_sustain_bitmask = 0;
                    memset(midi_voices, 0, sizeof(midi_voices));
                /*
                                for (int voice_number = 0; voice_number < MAX_MIDI_VOICES; ++voice_number)
                                    midi_voices[voice_number].playing = 0;
                */
                    break;
                case 0x79: // all controllers off
                    memset(midi_channels, 0, sizeof(midi_channel_t) * MIDI_CHANNELS);
                    break;
                default:
             //       debug_log("[MIDI] Unknown channel %i controller %02x %02x\n", channel, message->note, message->velocity);
                             
            }
            break;

        case 0xC: // Channel Program
            midi_channels[channel].program = message->note;

            for (int voice_slot = 0; voice_slot < MAX_MIDI_VOICES; ++voice_slot)
                if (midi_voices[voice_slot].channel == channel) {
                    CLEAR_ACTIVE_VOICE(voice_slot);
                }

       ///     debug_log("[MIDI] Channel %i program %i\n", message->command & 0xf, message->note);
            break;
        case 0xE: {
            const int pitch_bend = (message->velocity * 128 + message->note);
            const int cents = pitch_pows[pitch_bend];
            midi_channels[channel].pitch = cents;
          ///  debug_log("[MIDI] Channel %i pitch_bend %i cents %i 44000->%i\n", channel, pitch_bend - 8192, cents, apply_pitch(44000, cents));
                     
            for (int voice_slot = 0; voice_slot < MAX_MIDI_VOICES; ++voice_slot)
                if (midi_voices[voice_slot].channel == channel) {
                    midi_voices[voice_slot].frequency_m100 = apply_pitch(
                        midi_voices[voice_slot].frequency_m100, cents);
                }
            break;
        }
        default:
          //  debug_log("[MIDI] Unknown channel %i command %x message %04x \n", channel, message->command >> 4,midi_command);
                      
            break;
    }
}

//################################################################



enum { STATUS_READY = 0, STATUS_OUTPUT_NOT_READY = 0x40, STATUS_INPUT_NOT_READY = 0x80 };

uint8_t mpu_status = STATUS_INPUT_NOT_READY;
uint8_t mpu_rx_data = 0;

static int midi_pos, midi_len;
static uint32_t midi_command;
static int midi_lengths[8] = {3, 3, 3, 3, 2, 2, 3, 1};
static int midi_insysex;



uint8_t fast(mpu401_read)(void) {

    return mpu_status;

/*     if (portnum & 1) {
        return mpu_status;
    }

    mpu_status = STATUS_INPUT_NOT_READY;
    return mpu_rx_data; */
}

void fast(mpu401_write)( uint8_t value) {
/*     if (portnum & 1) {
        switch (value) {
            case 0xDF:
                mpu_rx_data = 0xFE; // Ack

                break;
            case 0xD0: // Enter UART Mode
                mpu_rx_data = 0xFF; // ???
                break;
            case 0x3F: // Enter Intellegent UART mode
                mpu_rx_data = 0xFE; // Ack
                break;
            default:

            case 0xFF: // Reset

                mpu_rx_data = 0xFE; // Ack
                break;
        }

        mpu_status = STATUS_READY;
        return;
    } */

    if (value & 0x80 && !(value == 0xF7 && midi_insysex)) {
        midi_pos = 0;
        midi_len = midi_lengths[value >> 4 & 7];
        midi_command = 0;

        if (value == 0xF0) midi_insysex = 1;
    }

    if (midi_insysex) {

        if (value == 0xF7) {
            midi_insysex = 0;
        }
        return;
    }

    if (midi_len) {
        midi_command |= value << midi_pos * 8;
        if (++midi_pos == midi_len) {

            parse_midi((void *)&midi_command);

        }
    }
}
//#######################################################################
void midi_off(void) // All Notes Off
{
    active_voice_bitmask = 0;
    channels_sustain_bitmask = 0;
    memset(midi_voices, 0, sizeof(midi_voices));
}
//#######################################################################

/*
; Driver for MIDI SAM2695 addon by ShamaZX
; #A0CF - control port
; #A1CF - data port


shama2695_prepare:
.reset_to_defined_state:
    ld bc, #a1cf ; if chip is already in parallel mode then sending 0x3F would switch it to serial mode.
    ld a, #ff    ; as we don't know initial chip state, we're just forcing it into serial mode by sending 0xFF command.
    out (c), a   ; after that we can safely use 0x3F command to get parallel mode.
    ld b, 20     ; see SAM2695 datasheet pages 13,19 for details
    djnz $       ;
.switch_to_parallel_mode:
    ld bc, #a1cf ;
    ld a, #3f    ;
    out (c), a   ;
    ld b, 20     ;
    djnz $       ;
    ld bc, #a0cf    ; we should read answer from data register otherwise parallel mode would be deactivated after 1ms
    in a, (c)    ; see SAM2695 datasheet page 8 for details
    ret          ;


shama2695_tx:
    ld e, a      ;
    ld bc, #a1cf ;
.wait:
    in a, (c)    ;
    and #40      ; check "receiver full" bit
    jr nz, .wait ; we can hang there forever...
.tx:
    ld b, #a0cf    ;
    out (c), e   ;
    .2 nop       ; minimum time between two consecutive writes must be 3.5 μs
    ret          ;


shama2695_flush_txbuf:
    ret          ;
*/

