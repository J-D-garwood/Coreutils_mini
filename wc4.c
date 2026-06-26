
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// Standard two-argument max macro. Parenthesised to be safe against operator
// precedence when expanded inside larger expressions. Note: arguments are
// evaluated twice — fine here because we only ever pass plain int variables.
#define MAX(a, b) (((a) > (b)) ? a : b)

// Indices into the bit-flag set as well as into the statistics[] array printed
// at the end of printStatistics. The order here is load-bearing: it must match
// the order of values pushed into statistics[] (lines, words, chars, bytes,
// longest), and the output loop iterates from line_count up to STAT_COUNT.
// STAT_COUNT is the sentinel "one past the last real flag" — both an array
// size and a loop bound.
enum statistics {line_count, word_count, char_count, byte_count, Longest_line, STAT_COUNT};

// Returns nonzero iff the bit at position `bit` is set in *flags.
// The "!= 0" normalises the masked value to a clean 0/1 boolean.
int check_bit(int * flags, int bit) {
    return (*flags & (1 << bit)) != 0;
}

// OR each of the requested bit positions into *flags. Used to apply the
// default set {-l -w -m} when no flags were given on the command line.
void set_bits(int * flags, int * bits, int len) {
    for (int i = 0; i < len; i++) {
        *flags = (*flags | (1 << bits[i]));
    }
}

// Classify a UTF-8 lead byte and return the total length of the sequence in
// bytes. The high bits of the lead byte encode the length per RFC 3629:
//   0xxxxxxx -> 1 byte  (ASCII)
//   110xxxxx -> 2 bytes
//   1110xxxx -> 3 bytes
//   11110xxx -> 4 bytes
// Continuation bytes (10xxxxxx) and other patterns are reported as invalid.
// The masks isolate the leading bits and compare against the expected prefix.
int get_utf8_length(unsigned char lead_byte) {
    if ((lead_byte & 0x80) == 0x00) return 1; //ASCII
    if ((lead_byte & 0xE0) == 0xC0) return 2; // 2-byte sequence
    if ((lead_byte & 0xF0) == 0xE0) return 3; // 3-byte sequence
    if ((lead_byte & 0xF8) == 0xF0) return 4; // 4-byte sequence
    return -1; //Invalid UTF-8 Lead byte
}

// Parse a single argv token as a short-option cluster like "-lwc".
// Returns 1 if the token was consumed as flags (so main should NOT treat it
// as a filename), and 0 if it isn't a flag token at all — meaning either it
// doesn't start with '-', it is the bare "-" (which conventionally means
// stdin), or it contained an unknown letter (in which case we also print a
// diagnostic to stderr).
//
// Bits are accumulated into `temp` first and only OR'd into *flags on
// success, so a partially-valid cluster like "-lZ" leaves *flags untouched.
int findFlags(int * flags, const char * s)
{
    int temp = 0;

    // Not a flag token: either doesn't start with '-' or is the lone "-".
    if (*s != '-' || *(s + 1) == '\0')
        return 0;

    int i = 1;
    char ch = *(s + i);

    // Walk each character after the leading '-' and set the matching bit.
    while (ch != '\0')
    {
        i++;
        if      (ch == 'c') temp |= (1 << byte_count);
        else if (ch == 'l') temp |= (1 << line_count);
        else if (ch == 'm') temp |= (1 << char_count);
        else if (ch == 'w') temp |= (1 << word_count);
        else if (ch == 'L') temp |= (1 << Longest_line);
        else {
            // Unknown letter: report and abandon this whole token.
            // temp is discarded so partial state doesn't leak into *flags.
            fprintf(stderr, "wc3: invalid flag '%c'\n", ch);
            return 0;
        }
        ch = *(s + i);
    }

    *flags |= temp;
    return 1;
}

// Recognise a small set of Unicode whitespace code points encoded as UTF-8.
// `character` points at the raw bytes of one already-decoded sequence and
// `len` is its byte length. Single-byte (ASCII) sequences are not handled
// here — the caller uses ctype's isspace() for those instead.
//
// This is deliberately a short allowlist of the most common "looks like a
// space" code points rather than a full Unicode whitespace table.
int isUTFwhitespace(char *character, int len) {
    if (len == 1) return 0;

    unsigned char seq1[] = {0xC2, 0xA0};             // NBSP
    unsigned char seq2[] = {0xE2, 0x80, 0x82};       // EN SPACE
    unsigned char seq3[] = {0xE2, 0x80, 0x83};       // EM SPACE
    unsigned char seq4[] = {0xE2, 0x80, 0x89};       // THIN SPACE
    unsigned char seq5[] = {0xE2, 0x80, 0x80};       // IDEOGRAPHIC SPACE

    // memcmp on the exact byte length avoids reading past the actual sequence.
    if (len == 2 && memcmp(character, seq1, 2) == 0) return 1;
    if (len == 3 && memcmp(character, seq2, 3) == 0) return 1;
    if (len == 3 && memcmp(character, seq3, 3) == 0) return 1;
    if (len == 3 && memcmp(character, seq4, 3) == 0) return 1;
    if (len == 3 && memcmp(character, seq5, 3) == 0) return 1;

    return 0;
}

// Read the entire stream `f` once, accumulate the five statistics, then print
// whichever ones are enabled in *flags followed by the filename label.
// Counts use a single pass with no buffering beyond what stdio provides.
void printStatistics(FILE * f, int * flags, char * filename)
{
    // Running totals. characters counts decoded Unicode characters (one per
    // UTF-8 sequence) while bytes counts raw bytes read from the stream.
    int lines = 0;
    int bytes = 0;
    int words = 0;
    int characters = 0;

    // -L tracking: longestLine is the best seen so far; currentLine grows as
    // we consume bytes within the current line and resets at '\n'.
    int longestLine = 0;
    int currentLine = 0;

    // Word counting uses a small state machine: a word begins on the
    // transition from whitespace to non-whitespace. was_space starts at 1
    // so that a file beginning with a non-space character correctly counts
    // its first word.
    int is_space_now = 0;
    int was_space = 1;

    int ch;                     // current byte just read (or EOF)
    int ch2;                    // continuation byte during multi-byte read
    int prev = '\0';            // previous character (used for -L: avoid
                                // counting the trailing newline of an empty
                                // line as a "current line" of length 0).

    char multi_byte_buffer[4];  // assembled UTF-8 sequence for the current
                                // character; only filled when charLen > 1.
    int charLen = 0;            // byte length of the current sequence

    int end = 0;                // set when EOF appears mid-multibyte so we
                                // can break out of the outer loop cleanly
                                // (the inner break only exits the for loop).

    while((ch = fgetc(f)) != EOF)
    {
        bytes++;
        charLen = get_utf8_length(ch);

        // Pull in the remaining continuation bytes of a multi-byte sequence.
        // If EOF hits in the middle we mark `end` and abandon: the partial
        // sequence isn't counted as a character and the loop terminates.
        if (charLen > 1) {
            multi_byte_buffer[0] = ch;
            for (int i = 0; i<charLen-1; i++) {
                if ((ch2 = fgetc(f)) == EOF) {
                    end = 1;
                    break;
                }
                multi_byte_buffer[i+1] = ch2;
                bytes++;
            }
        }

        if (end) break;

        // For multi-byte sequences consult our UTF-8 whitespace allowlist;
        // for single bytes (ASCII or invalid lead bytes treated as 1 byte)
        // defer to ctype. The cast to unsigned char is required because
        // isspace's behaviour on negative int values is undefined.
        is_space_now = charLen>1 ? isUTFwhitespace(multi_byte_buffer, charLen) : isspace((unsigned char)ch);

        // Word boundary: whitespace -> non-whitespace transition.
        if (!is_space_now && was_space) words++;
        characters++;
        currentLine++;

        // On newline, record the line length (excluding the '\n' itself, hence
        // currentLine-1). The prev != '\n' guard skips empty lines so a run
        // of blank lines doesn't repeatedly compare 0 against longestLine —
        // harmless but avoids the spurious "current line of length 0" idea.
        if (ch == '\n') {
            if (prev != '\n') longestLine = MAX(currentLine-1, longestLine);
            lines++;
            currentLine = 0;
        }

        was_space = is_space_now;
        prev = ch;
    }

    // File that ends without a trailing newline: the last line never went
    // through the '\n' branch, so account for it here. Same prev != '\n'
    // guard so we don't double-count a file that did end in '\n'.
    if (prev != '\n') longestLine = MAX(currentLine-1, longestLine);

    // Order MUST match the enum declared at the top of the file: the print
    // loop iterates enum positions 0..STAT_COUNT-1 and indexes into this
    // array with the same index.
    int statistics[] = {lines, words, characters, bytes, longestLine};

    for (int i = line_count; i < STAT_COUNT; i++)
    {
        if (check_bit(flags, i))
            printf(" %d", statistics[i]);
    }

    printf(" %s\n", filename);
    return;
}

int main(int argc, char * argv[])
{
    int flags = 0;

    // Default flag set used when the user passes no -lwcmL options.
    int default_bits[] = {line_count, word_count, char_count};

    // VLA sized by argc: at most argc-1 positional args (filenames). Cheap
    // and avoids a malloc, but not portable to compilers without C99 VLAs.
    char * pos[argc];
    int npos = 0;

    // Single pass over argv: anything findFlags consumes is a flag token;
    // everything else (including "-") falls through to the positional list.
    for (int i = 1; i < argc; i++)
    {
        if (!findFlags(&flags, argv[i]))
            pos[npos++] = argv[i];
    }

    FILE * f;
    char * filename;

    // No flags supplied -> use the conventional default set.
    if (flags == 0) set_bits(&flags, default_bits, 3);

    if (npos == 0) {
        // No filenames given: read from stdin and label it accordingly.
        f = stdin;
        filename = "stdin";
        printStatistics(f, &flags, filename);
    } else {
        for (int i = 0; i < npos; i++)
        {
            // "-" is the conventional alias for stdin in coreutils.
            if (strcmp(pos[i], "-") != 0) {
                f = fopen(pos[i], "r");
                filename = pos[i];
            } else {
                f = stdin;
                filename = "stdin";
            }
            // fopen failure is reported per-file and we move on, so a bad
            // path in the middle of a list doesn't suppress the others.
            if (f == NULL) {
                fprintf(stderr, "wc3: %s: %s\n", pos[i], strerror(errno));
                continue;
            } else {
                printStatistics(f, &flags, filename);
                // Don't fclose stdin — it isn't ours to close.
                if (f != stdin) fclose(f);
            }
        }
    }
    return 0;
}
