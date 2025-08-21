#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

// Define strdup if not available
#ifndef _GNU_SOURCE
char *strdup(const char *s) {
    char *d = malloc(strlen(s) + 1);
    if (d != NULL) strcpy(d, s);
    return d;
}
#endif

#define MAX_STRING_LENGTH 4096

// Simple data structures
typedef struct area_data {
    char *name;
    char *file_name;
    char *credits;
    char *builders;
} AREA_DATA;

typedef struct affect_data {
    int where;
    int type;
    int level;
    int duration;
    int location;
    int modifier;
    long bitvector;
    struct affect_data *next;
} AFFECT_DATA;

typedef struct extra_descr_data {
    char *keyword;
    char *description;
    struct extra_descr_data *next;
} EXTRA_DESCR_DATA;

typedef struct affect_out {
    char type[16]; // "normal", "flag", "spell"
    char location[32];
    int modifier;
    char extra[64]; // for spell name or flag name if needed
    struct affect_out *next;
} AFFECT_OUT;

typedef struct obj_index_data {
    long vnum;
    char *name;
    char *short_descr;
    char *description;
    char *material;
    int item_type;
    int extra_flags;
    char *extra_flags_str; // Store extra flags as string from area file
    int wear_flags;


    int level;
    int condition;
    int weight;
    int cost;
    int value[5];
    char *materia_spell; // For materia items
    char *weapon_type; // For weapon items
    char *damage_type; // For weapon items
    char *weapon_flags; // For weapon items
    AFFECT_DATA *affected;
    AFFECT_DATA *affected2;
    EXTRA_DESCR_DATA *extra_descr;
    AREA_DATA *area;
    AFFECT_OUT *affects_out;
    struct obj_index_data *next;
} OBJ_INDEX_DATA;

AREA_DATA *current_area = NULL;
OBJ_INDEX_DATA *object_list = NULL;

// --- Lookup tables and flag-to-string logic ---

// Item type table
static const struct { int type; const char *name; } item_type_table[] = {
    {1, "light"}, {2, "scroll"}, {3, "wand"}, {4, "staff"}, {5, "weapon"},
    {6, "shard"}, {7, "ticket"}, {8, "treasure"}, {9, "armor"}, {10, "potion"},
    {11, "clothing"}, {12, "furniture"}, {13, "trash"}, {15, "container"}, {17, "drink_con"},
    {18, "key"}, {19, "food"}, {20, "money"}, {22, "boat"}, {23, "corpse_npc"},
    {24, "corpse_pc"}, {25, "fountain"}, {26, "pill"}, {27, "protect"}, {28, "map"},
    {29, "portal"}, {30, "warp_stone"}, {31, "room_key"}, {32, "gem"}, {33, "jewelry"},
    {34, "jukebox"}, {35, "quiver"}, {36, "arrow"}, {37, "poison"}, {38, "disjunction"},
    {39, "safe_haven"}, {40, "materia"}, {41, "remote"}, {46, "scryer"}, {47, "exit"},
    {48, "minigame"}, {0, NULL}
};

// Wear flags (bit positions) - complete list from merc.h
// A=0, B=1, C=2, D=3, E=4, F=5, G=6, H=7, I=8, J=9, K=10, L=11, M=12, N=13, O=14, P=15
// Q=16, R=17, S=18, T=19, U=20, V=21, W=22, X=23, Y=24, Z=25, aa=26, bb=27, cc=28, dd=29
static const char *wear_flag_table[] = {
    "take", "finger", "neck", "body", "head", "legs", "feet", "hands", "arms", "shield",
    "about", "waist", "wrist", "wield", "hold", "nosac", "wearfloat", "face", "lodge_leg", "lodge_arm",
    "lodge_rib", "materia", "nose", "belly", "ears", "tongue", "tattoo", "gadget", "grimoire", "familiar", NULL
};

// Extra flags (from tables.c - exact order)
static const char *extra_flag_table[] = {
    "glow", "hum", "dark", "lock", "evil", "invis", "magic", "nodrop", "bless", "antigood",
    "antievil", "antineutral", "noremove", "inventory", "nopurge", "rotdeath", "visdeath", "noclone", "nonmetal", "nolocate",
    "meltdrop", "hadtimer", "sellextract", "clan", "burnproof", "nouncurse", "sticky", "lodged", "trap", "no_restring",
    "quest", "nogive", NULL
};

// Affect locations (from merc.h - exact order)
// Affect locations (EXACT match to APPLY_* constants in merc.h)
static const char *affect_location_table[] = {
    "none", 
    "strength", 
    "dexterity", 
    "intelligence", 
    "wisdom", 
    "constitution", 
    "sex", 
    "class", 
    "level", 
    "age", 
    "height", 
    "weight", 
    "mana", 
    "hp", 
    "move", 
    "gold", 
    "experience",
    "ac", 
    "hitroll", 
    "damroll", 
    "saves", 
    "savingrod", 
    "savingpetri", 
    "savingbreath", 
    "savingspell", 
    "spellaffect", 
    "spellcast", 
    "resistance", 
    "critchance", 
    "critdamage", 
    "recuperation", 
    "concentration", 
    "prosperity", 
    "endurance", 
    "penetration", 
    "alacrity", 
    "insight", 
    "celerity", 
    "potency", 
    "savingpara", 
    "bounty",
    NULL
};

// Shield flags (from tables.c)
static const char *shield_flags[] = {
    "living_armor", "sanctuary", "invisible", "protect_evil", "protect_good", 
    "planeshift", "fireshield", "pass_door", "protect_voodoo", "iceshield",
    "lightningshield", "acidshield", NULL
};

// Affect flags (from tables.c, corrected to match AFF_* definitions)
static const char *affect_flags[] = {
    "blind", "detect_evil", "detect_invis", "detect_magic", "detect_hidden", "detect_good", "unused_1", "unused_h", "faerie_fire", "infrared", "curse", "resistance", "poison", "unused_2", "unused_3", "sneak", "hide", "sleep", "charm", "flying", "unused_4", "haste", "calm", "plague", "weaken", "dark_vision", "berserk", "swim", "regeneration", "slow", "drained", NULL
};

// Affect2 flags (from tables.c, corrected to match AFF_* definitions)
static const char *affect2_flags[] = {
    "shapeshift", "unused_1", "telepathy", "life_stealer", "unused_e", "lsd", "hold_person", 
    "unused_2", "divine_intervention", "unused_3", "mental_disruption", "talon", "kamikaze", 
    "spiritlink", "unused_4", "unused_5", "unused_6", "unused_7", "unused_8", "unused_9", 
    "unused_10", "unused_11", "spectral_blade", "unused_12", "unused_13", "unused_14", 
    "focus_chi", NULL
};

// Immune flags (from tables.c)
static const char *imm_flags[] = {
    "summon", "charm", "magic", "weapon", "bash", "pierce", "slash", "fire", "cold", "lightning", "acid", "poison", "negative", "holy", "energy", "mental", "disease", "drowning", "light", "sound", "wood", "silver", "iron", NULL
};

// Resist flags (from tables.c)
static const char *res_flags[] = {
    "summon", "charm", "magic", "weapon", "bash", "pierce", "slash", "fire", "cold", "lightning", "acid", "poison", "negative", "holy", "energy", "mental", "disease", "drowning", "light", "sound", "wood", "silver", "iron", NULL
};

// Vulnerable flags (from tables.c)
static const char *vuln_flags[] = {
    "summon", "charm", "magic", "weapon", "bash", "pierce", "slash", "fire", "cold", "lightning", "acid", "poison", "negative", "holy", "energy", "mental", "disease", "drowning", "light", "sound", "wood", "silver", "iron", NULL
};

// Weapon flags (from tables.c)
static const char *weapon_flags[] = {
    "flaming", "frost", "vampiric", "sharp", "vorpal", "two_hands", "shocking", "poisoned", NULL
};

// Helper: get item type name
const char *item_type_name(int type) {
    for (int i = 0; item_type_table[i].name; ++i)
        if (item_type_table[i].type == type)
            return item_type_table[i].name;
    return "unknown";
}

// Helper: convert bitfield to flag string (for wear/extra flags)
void bitfield_to_names(int bits, const char **table, char *out, size_t outlen) {
    out[0] = '\0';
    int first = 1;
    for (int i = 0; table[i]; ++i) {
        if (bits & (1 << i)) {
            if (!first) strncat(out, " ", outlen - strlen(out) - 1);
            strncat(out, table[i], outlen - strlen(out) - 1);
            first = 0;
        }
    }
    if (first) strncat(out, "none", outlen - strlen(out) - 1);
}

// Helper: affect location name
const char *affect_location_name(int loc) {
    if (loc >= 0 && loc < 41 && affect_location_table[loc])
        return affect_location_table[loc];
    return "unknown";
}

// Helper: convert shield bitvector to name
const char *shield_bit_name(int bits) {
    for (int i = 0; shield_flags[i]; ++i) {
        if (bits & (1 << i)) {
            return shield_flags[i];
        }
    }
    return "unknown";
}

// Helper: convert affect bitvector to name
const char *affect_bit_name(int bits) {
    for (int i = 0; affect_flags[i]; ++i) {
        if (bits & (1 << i)) {
            return affect_flags[i];
        }
    }
    return "unknown";
}

// Helper: convert affect2 bitvector to name
const char *affect2_bit_name(int bits) {
    for (int i = 0; affect2_flags[i]; ++i) {
        if (bits & (1 << i)) {
            return affect2_flags[i];
        }
    }
    return "unknown";
}

// Helper: convert immune bitvector to name
const char *immune_bit_name(int bits) {
    for (int i = 0; imm_flags[i]; ++i) {
        if (bits & (1 << i)) {
            return imm_flags[i];
        }
    }
    return "unknown";
}

// Helper: convert resist bitvector to name
const char *resist_bit_name(int bits) {
    for (int i = 0; res_flags[i]; ++i) {
        if (bits & (1 << i)) {
            return res_flags[i];
        }
    }
    return "unknown";
}

// Helper: convert vulnerable bitvector to name
const char *vuln_bit_name(int bits) {
    for (int i = 0; vuln_flags[i]; ++i) {
        if (bits & (1 << i)) {
            return vuln_flags[i];
        }
    }
    return "unknown";
}

// Helper: convert weapon bitvector to name
const char *weapon_bit_name(int bits) {
    for (int i = 0; weapon_flags[i]; ++i) {
        if (bits & (1 << i)) {
            return weapon_flags[i];
        }
    }
    return "unknown";
}

// Weapon type lookup
const char *weapon_type_name(int type) {
    switch (type) {
        case 0: return "exotic";
        case 1: return "sword";
        case 2: return "dagger";
        case 3: return "spear";
        case 4: return "mace";
        case 5: return "axe";
        case 6: return "flail";
        case 7: return "whip";
        case 8: return "polearm";
        case 9: return "bow";
        default: return "unknown";
    }
}

// Damage type lookup
const char *damage_type_name(int type) {
    switch (type) {
        case 0: return "none";
        case 1: return "slice";
        case 2: return "stab";
        case 3: return "slash";
        case 4: return "whip";
        case 5: return "claw";
        case 6: return "blast";
        case 7: return "pound";
        case 8: return "crush";
        case 9: return "grep";
        case 10: return "bite";
        case 11: return "pierce";
        case 12: return "suction";
        case 13: return "beating";
        case 14: return "digestion";
        case 15: return "charge";
        case 16: return "slap";
        case 17: return "punch";
        case 18: return "wrath";
        case 19: return "magic";
        case 20: return "divine";
        case 21: return "kiss";
        case 22: return "cleave";
        case 23: return "scratch";
        case 24: return "peck";
        case 25: return "peckb";
        case 26: return "chop";
        case 27: return "sting";
        case 28: return "smash";
        case 29: return "shbite";
        case 30: return "flbite";
        case 31: return "frbite";
        case 32: return "acbite";
        case 33: return "chomp";
        case 34: return "drain";
        case 35: return "thrust";
        case 36: return "slime";
        case 37: return "shock";
        case 38: return "thwack";
        case 39: return "flame";
        case 40: return "chill";
        case 41: return "poison";
        case 42: return "pulse";
        case 43: return "bleed";
        default: return "unknown";
    }
}

// Weapon flags lookup - using actual weapon flags from merc.h
const char *weapon_flag_name(int flag) {
    switch (flag) {
        case 0: return "none";
        case 1: return "flaming";      // A
        case 2: return "frost";        // B
        case 4: return "vampiric";     // C
        case 8: return "sharp";        // D
        case 16: return "vorpal";      // E
        case 32: return "two_hands";   // F
        case 64: return "shocking";    // G
        case 128: return "poison";     // H
        case 256: return "acid";       // I
        case 1024: return "purify";    // K
        default: return "unknown";
    }
}

char *escape_json_string(const char *input) {
    if (!input) return strdup("");
    
    // Calculate required length
    int len = 0;
    for (const char *p = input; *p; p++) {
        switch (*p) {
            case '"':  len += 2; break;  // \" 
            case '\\': len += 2; break;  // \\
            case '\b': len += 2; break;  // \b
            case '\f': len += 2; break;  // \f
            case '\n': len += 2; break;  // \n
            case '\r': len += 2; break;  // \r
            case '\t': len += 2; break;  // \t
            default:   len += 1; break;
        }
    }
    
    char *output = malloc(len + 1);
    char *q = output;
    
    for (const char *p = input; *p; p++) {
        switch (*p) {
            case '"':  *q++ = '\\'; *q++ = '"';  break;
            case '\\': *q++ = '\\'; *q++ = '\\'; break;
            case '\b': *q++ = '\\'; *q++ = 'b';  break;
            case '\f': *q++ = '\\'; *q++ = 'f';  break;
            case '\n': *q++ = '\\'; *q++ = 'n';  break;
            case '\r': *q++ = '\\'; *q++ = 'r';  break;
            case '\t': *q++ = '\\'; *q++ = 't';  break;
            default:   *q++ = *p; break;
        }
    }
    *q = '\0';
    return output;
}

// Convert extra flag letters to full names
char *extra_flags_to_names(const char *flags_str) {
    if (!flags_str || !*flags_str) return strdup("none");
    
    static char result[1024];
    char *p = result;
    int first = 1;
    
    for (const char *c = flags_str; *c; c++) {
        const char *flag_name = NULL;
        
        switch (*c) {
            case 'A': flag_name = "glow"; break;
            case 'B': flag_name = "hum"; break;
            case 'C': flag_name = "dark"; break;
            case 'D': flag_name = "lock"; break;
            case 'E': flag_name = "evil"; break;
            case 'F': flag_name = "invis"; break;
            case 'G': flag_name = "magic"; break;
            case 'H': flag_name = "nodrop"; break;
            case 'I': flag_name = "bless"; break;
            case 'J': flag_name = "antigood"; break;
            case 'K': flag_name = "antievil"; break;
            case 'L': flag_name = "antineutral"; break;
            case 'M': flag_name = "noremove"; break;
            case 'N': flag_name = "inventory"; break;
            case 'O': flag_name = "nopurge"; break;
            case 'P': flag_name = "rot_death"; break;
            case 'Q': flag_name = "vis_death"; break;
            case 'R': flag_name = "no_locate"; break;
            case 'S': flag_name = "meltdrop"; break;
            case 'T': flag_name = "had_charge"; break;
            case 'U': flag_name = "sellextract"; break;
            case 'V': flag_name = "quest"; break;
            case 'W': flag_name = "questpoint"; break;
            case 'X': flag_name = "no_auction"; break;
            case 'Y': flag_name = "burnproof"; break;
            case 'Z': flag_name = "no_give"; break;
            case 'a': flag_name = "no_locate2"; break;
            case 'b': flag_name = "no_sac"; break;
            case 'c': flag_name = "no_drop2"; break;
            case 'd': flag_name = "no_restring"; break;
            case 'e': flag_name = "no_junk"; break;
            case 'f': flag_name = "no_sell"; break;
            case 'g': flag_name = "no_store"; break;
            case 'h': flag_name = "no_purge"; break;
            default: flag_name = "unknown"; break;
        }
        
        if (!first) {
            *p++ = ' ';
        }
        first = 0;
        
        while (*flag_name) *p++ = *flag_name++;
    }
    
    *p = '\0';
    
    return strdup(result);
}



// Convert weapon flag letters to full names - using actual weapon flags from merc.h
char *weapon_flags_to_names(const char *flags_str) {
    if (!flags_str || !*flags_str) return strdup("[]");
    
    static char result[2048];
    char *p = result;
    int first = 1;
    
    *p++ = '[';
    
    for (const char *c = flags_str; *c; c++) {
        const char *flag_name = NULL;
        
        switch (*c) {
            case 'A': flag_name = "flaming"; break;
            case 'B': flag_name = "frost"; break;
            case 'C': flag_name = "vampiric"; break;
            case 'D': flag_name = "sharp"; break;
            case 'E': flag_name = "vorpal"; break;
            case 'F': flag_name = "two_hands"; break;
            case 'G': flag_name = "shocking"; break;
            case 'H': flag_name = "poison"; break;
            case 'I': flag_name = "acid"; break;
            case 'K': flag_name = "purify"; break;
            default: flag_name = "unknown"; break;
        }
        
        if (!first) {
            *p++ = ',';
            *p++ = ' ';
        }
        first = 0;
        
        *p++ = '"';
        strcpy(p, flag_name);
        p += strlen(flag_name);
        *p++ = '"';
    }
    
    *p++ = ']';
    *p = '\0';
    
    return strdup(result);
}

// File reading functions
char fread_letter(FILE *fp) {
    char c;
    do {
        c = getc(fp);
        if (c == EOF) return EOF;
    } while (isspace(c));
    return c;
}

int fread_number(FILE *fp) {
    int number = 0;
    char c;
    bool negative = false;
    
    do {
        c = getc(fp);
        if (c == EOF) return 0;
    } while (isspace(c));
    
    if (c == '-') {
        negative = true;
        c = getc(fp);
    }
    
    if (!isdigit(c)) return 0;
    
    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }
    
    if (c != ' ') ungetc(c, fp);
    
    if (negative)
        return -1 * number;
    
    return number;
}

long flag_convert(char letter) {
    long bitsum = 0;
    char i;

    if ('A' <= letter && letter <= 'Z') {
        bitsum = 1;
        for (i = letter; i > 'A'; i--)
            bitsum *= 2;
    } else if ('a' <= letter && letter <= 'z') {
        bitsum = 67108864; /* 2^26 */
        for (i = letter; i > 'a'; i--)
            bitsum *= 2;
    }

    return bitsum;
}

static long flag_string_to_bits(const char *str) {
    long bits = 0;
    for (const char *p = str; *p; ++p) {
        if (('A' <= *p && *p <= 'Z') || ('a' <= *p && *p <= 'z')) {
            bits += flag_convert(*p);
        }
    }
    return bits;
}

long fread_flag(FILE *fp) {
    int number = 0;
    char c;
    bool negative = false;

    do {
        c = getc(fp);
        if (c == EOF) return 0;
    } while (isspace(c));

    if (c == '-') {
        negative = true;
        c = getc(fp);
    }

    number = 0;

    if (!isdigit(c)) {
        while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
            number += flag_convert(c);
            c = getc(fp);
        }
        // Stop at newline, carriage return, or space
        if (c == '\n' || c == '\r' || c == ' ') {
            ungetc(c, fp);
            return number;
        }
        // If we hit any other character, put it back and return
        if (c != EOF) {
            ungetc(c, fp);
        }
        return number;
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (c == '|')
        number += fread_flag(fp);
    else if (c != ' ')
        ungetc(c, fp);

    if (negative)
        return -1 * number;

    return number;
}

char *fread_string(FILE *fp) {
    static char buffer[MAX_STRING_LENGTH];
    char *p = buffer;
    char c;
    
    do {
        c = getc(fp);
        if (c == EOF) return strdup("");
    } while (isspace(c));
    
    if (c == '~') return strdup("");
    
    do {
        *p++ = c;
        c = getc(fp);
    } while (c != '~' && c != EOF && p < buffer + MAX_STRING_LENGTH - 1);
    
    *p = '\0';
    return strdup(buffer);
}

char *fread_word(FILE *fp) {
    static char buffer[MAX_STRING_LENGTH];
    char *p = buffer;
    char c;
    
    do {
        c = getc(fp);
        if (c == EOF) return NULL;
    } while (isspace(c));
    
    do {
        *p++ = c;
        c = getc(fp);
    } while (!isspace(c) && c != EOF && p < buffer + MAX_STRING_LENGTH - 1);
    
    *p = '\0';
    return strdup(buffer);
}

void fread_to_eol(FILE *fp) {
    char c;
    do {
        c = getc(fp);
    } while (c != '\n' && c != EOF);
}

// Item type lookup
int item_lookup(const char *name) {
    if (!strcmp(name, "light")) return 1;
    if (!strcmp(name, "scroll")) return 2;
    if (!strcmp(name, "wand")) return 3;
    if (!strcmp(name, "staff")) return 4;
    if (!strcmp(name, "weapon")) return 5;
    if (!strcmp(name, "shard")) return 6;
    if (!strcmp(name, "ticket")) return 7;
    if (!strcmp(name, "treasure")) return 8;
    if (!strcmp(name, "armor")) return 9;
    if (!strcmp(name, "potion")) return 10;
    if (!strcmp(name, "clothing")) return 11;
    if (!strcmp(name, "furniture")) return 12;
    if (!strcmp(name, "trash")) return 13;
    if (!strcmp(name, "container")) return 15;
    if (!strcmp(name, "drink_con")) return 17;
    if (!strcmp(name, "key")) return 18;
    if (!strcmp(name, "food")) return 19;
    if (!strcmp(name, "money")) return 20;
    if (!strcmp(name, "boat")) return 22;
    if (!strcmp(name, "corpse_npc")) return 23;
    if (!strcmp(name, "corpse_pc")) return 24;
    if (!strcmp(name, "fountain")) return 25;
    if (!strcmp(name, "pill")) return 26;
    if (!strcmp(name, "protect")) return 27;
    if (!strcmp(name, "map")) return 28;
    if (!strcmp(name, "portal")) return 29;
    if (!strcmp(name, "warp_stone")) return 30;
    if (!strcmp(name, "room_key")) return 31;
    if (!strcmp(name, "gem")) return 32;
    if (!strcmp(name, "jewelry")) return 33;
    if (!strcmp(name, "jukebox")) return 34;
    if (!strcmp(name, "quiver")) return 35;
    if (!strcmp(name, "arrow")) return 36;
    if (!strcmp(name, "poison")) return 37;
    if (!strcmp(name, "disjunction")) return 38;
    if (!strcmp(name, "safe_haven")) return 39;
    if (!strcmp(name, "materia")) return 40;
    if (!strcmp(name, "remote")) return 41;
    if (!strcmp(name, "scryer")) return 46;
    if (!strcmp(name, "exit")) return 47;
    if (!strcmp(name, "minigame")) return 48;
    return 0;
}

// Weapon type lookup function
int weapon_type_lookup(const char *name) {
    if (!name) return 0;
    
    if (!strcmp(name, "exotic")) return 0;
    if (!strcmp(name, "sword")) return 1;
    if (!strcmp(name, "dagger")) return 2;
    if (!strcmp(name, "spear")) return 3;
    if (!strcmp(name, "staff")) return 3; // Staff maps to spear in the MUD
    if (!strcmp(name, "mace")) return 4;
    if (!strcmp(name, "axe")) return 5;
    if (!strcmp(name, "flail")) return 6;
    if (!strcmp(name, "whip")) return 7;
    if (!strcmp(name, "polearm")) return 8;
    if (!strcmp(name, "bow")) return 9;
    
    return 0; // Default to exotic
}

// Spell name lookup function - using actual MUD spell list
int spell_lookup(const char *name) {
    if (!name) return 0;
    
    // Spell mappings based on actual MUD spell list
    if (!strcmp(name, "reserved")) return 0;
    if (!strcmp(name, "hallucination")) return 1;
    if (!strcmp(name, "acid blast")) return 2;
    if (!strcmp(name, "armor")) return 3;
    if (!strcmp(name, "bless")) return 4;
    if (!strcmp(name, "blindness")) return 5;
    if (!strcmp(name, "burning hands")) return 6;
    if (!strcmp(name, "call lightning")) return 7;
    if (!strcmp(name, "calm")) return 8;
    if (!strcmp(name, "cancellation")) return 9;
    if (!strcmp(name, "cause critical")) return 10;
    if (!strcmp(name, "cause discord")) return 11;
    if (!strcmp(name, "cause light")) return 12;
    if (!strcmp(name, "cause serious")) return 13;
    if (!strcmp(name, "chain lightning")) return 14;
    if (!strcmp(name, "change sex")) return 15;
    if (!strcmp(name, "charm person")) return 16;
    if (!strcmp(name, "chill touch")) return 17;
    if (!strcmp(name, "colour spray")) return 18;
    if (!strcmp(name, "continual light")) return 19;
    if (!strcmp(name, "control weather")) return 20;
    if (!strcmp(name, "call demon")) return 21;
    if (!strcmp(name, "create golem")) return 22;
    if (!strcmp(name, "guardian spirit")) return 23;
    if (!strcmp(name, "call servant")) return 24;
    if (!strcmp(name, "create food")) return 25;
    if (!strcmp(name, "create rose")) return 26;
    if (!strcmp(name, "create spring")) return 27;
    if (!strcmp(name, "create water")) return 28;
    if (!strcmp(name, "cure blindness")) return 29;
    if (!strcmp(name, "cure critical")) return 30;
    if (!strcmp(name, "cure disease")) return 31;
    if (!strcmp(name, "psychic healing")) return 32;
    if (!strcmp(name, "poke")) return 33;
    if (!strcmp(name, "crush")) return 34;
    if (!strcmp(name, "tickle")) return 35;
    if (!strcmp(name, "essence")) return 36;
    if (!strcmp(name, "prayer")) return 37;
    if (!strcmp(name, "cure light")) return 38;
    if (!strcmp(name, "cure poison")) return 39;
    if (!strcmp(name, "cure serious")) return 40;
    if (!strcmp(name, "curse")) return 41;
    if (!strcmp(name, "demonfire")) return 42;
    if (!strcmp(name, "hellfire")) return 43;
    if (!strcmp(name, "permanency")) return 44;
    if (!strcmp(name, "fireshield")) return 45;
    if (!strcmp(name, "detect weakness")) return 46;
    if (!strcmp(name, "detect evil")) return 47;
    if (!strcmp(name, "detect good")) return 48;
    if (!strcmp(name, "detect hidden")) return 49;
    if (!strcmp(name, "detect invis")) return 50;
    if (!strcmp(name, "detect magic")) return 51;
    if (!strcmp(name, "detect poison")) return 52;
    if (!strcmp(name, "dispel evil")) return 53;
    if (!strcmp(name, "dispel good")) return 54;
    if (!strcmp(name, "dispel magic")) return 55;
    if (!strcmp(name, "fissure")) return 56;
    if (!strcmp(name, "earthquake")) return 57;
    if (!strcmp(name, "animate dead")) return 58;
    if (!strcmp(name, "enchant armor")) return 59;
    if (!strcmp(name, "empower armor")) return 60;
    if (!strcmp(name, "dark ritual")) return 61;
    if (!strcmp(name, "brand")) return 62;
    if (!strcmp(name, "empower weapon")) return 63;
    if (!strcmp(name, "enchant weapon")) return 64;
    if (!strcmp(name, "disjunction")) return 65;
    if (!strcmp(name, "safe haven")) return 66;
    if (!strcmp(name, "alternate dimension")) return 67;
    if (!strcmp(name, "hold person")) return 68;
    if (!strcmp(name, "entangle")) return 69;
    if (!strcmp(name, "splinter storm")) return 70;
    if (!strcmp(name, "energy drain")) return 71;
    if (!strcmp(name, "magic drain")) return 72;
    if (!strcmp(name, "faerie fire")) return 73;
    if (!strcmp(name, "faerie fog")) return 74;
    if (!strcmp(name, "farsight")) return 75;
    if (!strcmp(name, "fireball")) return 76;
    if (!strcmp(name, "mental blast")) return 77;
    if (!strcmp(name, "mental disruption")) return 78;
    if (!strcmp(name, "life drain")) return 79;
    if (!strcmp(name, "energy syphon")) return 80;
    if (!strcmp(name, "meteor")) return 81;
    if (!strcmp(name, "fireproof")) return 82;
    if (!strcmp(name, "flamestrike")) return 83;
    if (!strcmp(name, "fly")) return 84;
    if (!strcmp(name, "floating disc")) return 85;
    if (!strcmp(name, "frenzy")) return 86;
    if (!strcmp(name, "divine favor")) return 87;
    if (!strcmp(name, "divine intervention")) return 88;
    if (!strcmp(name, "gate")) return 89;
    if (!strcmp(name, "giant strength")) return 90;
    if (!strcmp(name, "harm")) return 91;
    if (!strcmp(name, "haste")) return 92;
    if (!strcmp(name, "heal")) return 93;
    if (!strcmp(name, "heat metal")) return 94;
    if (!strcmp(name, "holy word")) return 95;
    if (!strcmp(name, "divine power")) return 96;
    if (!strcmp(name, "wrath")) return 97;
    if (!strcmp(name, "identify")) return 98;
    if (!strcmp(name, "infravision")) return 99;
    if (!strcmp(name, "invisibility")) return 100;
    if (!strcmp(name, "know alignment")) return 101;
    if (!strcmp(name, "lightning bolt")) return 102;
    if (!strcmp(name, "remote view")) return 103;
    if (!strcmp(name, "raven spy")) return 104;
    if (!strcmp(name, "locate object")) return 105;
    if (!strcmp(name, "magic missile")) return 106;
    if (!strcmp(name, "mass healing")) return 107;
    if (!strcmp(name, "ice storm")) return 108;
    if (!strcmp(name, "mass invis")) return 109;
    if (!strcmp(name, "nexus")) return 110;
    if (!strcmp(name, "pass door")) return 111;
    if (!strcmp(name, "plague")) return 112;
    if (!strcmp(name, "poison")) return 113;
    if (!strcmp(name, "portal")) return 114;
    if (!strcmp(name, "protection evil")) return 115;
    if (!strcmp(name, "protection good")) return 116;
    if (!strcmp(name, "ray of truth")) return 117;
    if (!strcmp(name, "recharge")) return 118;
    if (!strcmp(name, "refresh")) return 119;
    if (!strcmp(name, "remove curse")) return 120;
    if (!strcmp(name, "telepathy")) return 121;
    if (!strcmp(name, "life stealer")) return 122;
    if (!strcmp(name, "sanctuary")) return 123;
    if (!strcmp(name, "shapeshift")) return 124;
    if (!strcmp(name, "living armor")) return 125;
    if (!strcmp(name, "trembling earth")) return 126;
    if (!strcmp(name, "planeshift")) return 127;
    if (!strcmp(name, "protective sphere")) return 128;
    if (!strcmp(name, "bark skin")) return 129;
    if (!strcmp(name, "talon")) return 130;
    if (!strcmp(name, "shield")) return 131;
    if (!strcmp(name, "shocking grasp")) return 132;
    if (!strcmp(name, "sleep")) return 133;
    if (!strcmp(name, "slow")) return 134;
    if (!strcmp(name, "stone skin")) return 135;
    if (!strcmp(name, "summon")) return 136;
    if (!strcmp(name, "teleport")) return 137;
    if (!strcmp(name, "ventriloquate")) return 138;
    if (!strcmp(name, "weaken")) return 139;
    if (!strcmp(name, "word of recall")) return 140;
    if (!strcmp(name, "mallocs empower")) return 141;
    if (!strcmp(name, "caines maddness")) return 142;
    if (!strcmp(name, "dinchaks power")) return 143;
    if (!strcmp(name, "acid breath")) return 144;
    if (!strcmp(name, "fire breath")) return 145;
    if (!strcmp(name, "frost breath")) return 146;
    if (!strcmp(name, "gas breath")) return 147;
    if (!strcmp(name, "lightning breath")) return 148;
    if (!strcmp(name, "general purpose")) return 149;
    if (!strcmp(name, "high explosive")) return 150;
    if (!strcmp(name, "imprint")) return 151;
    if (!strcmp(name, "avalons protection")) return 152;
    if (!strcmp(name, "psychic influence")) return 153;
    if (!strcmp(name, "spectral blade")) return 154;
    
    return 0; // Unknown spell
}

// Load objects - EXACT MUD LOGIC
void load_objects(FILE *fp) {
    for (;;) {
        long vnum;
        char letter;
        OBJ_INDEX_DATA *pObjIndex;

        letter = fread_letter(fp);
        if (letter == EOF) break;
        if (letter != '#') {
            fprintf(stderr, "Load_objects: # not found, got '%c'\n", letter);
            break;
        }

        vnum = fread_number(fp);
        fprintf(stderr, "Loading object vnum: %ld\n", vnum);
        if (vnum == 0) {
            // End of objects section - read until next #0 or section
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    char *next_word = fread_word(fp);
                    if (next_word && !strcmp(next_word, "0")) {
                        free(next_word);
                        break;
                    }
                    if (next_word) free(next_word);
                }
            }
            break;
        }

        pObjIndex = malloc(sizeof(OBJ_INDEX_DATA));
        pObjIndex->vnum = vnum;
        pObjIndex->area = current_area;
        pObjIndex->name = fread_string(fp);
        fprintf(stderr, "Read name: %s\n", pObjIndex->name);
        pObjIndex->short_descr = fread_string(fp);
        fprintf(stderr, "Read short_descr: %s\n", pObjIndex->short_descr);
        pObjIndex->description = fread_string(fp);
        fprintf(stderr, "Read description: %s\n", pObjIndex->description);
        pObjIndex->material = fread_string(fp);
        fprintf(stderr, "Read material: %s\n", pObjIndex->material);

        // Read item type as string and convert
        char *item_type_str = fread_word(fp);
        pObjIndex->item_type = item_lookup(item_type_str);
        fprintf(stderr, "Read item_type_str: '%s', converted to: %d\n", item_type_str, pObjIndex->item_type);
        free(item_type_str);
        pObjIndex->extra_flags = fread_flag(fp);
        fprintf(stderr, "Read extra_flags: %d\n", pObjIndex->extra_flags);
        pObjIndex->wear_flags = fread_flag(fp);
        fprintf(stderr, "Read wear_flags: %d\n", pObjIndex->wear_flags);

        // Read values based on item type
        if (pObjIndex->item_type == 40) { // ITEM_MATERIA
            pObjIndex->value[0] = fread_number(fp);
            // Read spell name delimited by single quotes
            char c = fread_letter(fp);
            if (c == '\'') {
                static char spell_buffer[256];
                char *p = spell_buffer;
                while ((c = getc(fp)) != '\'' && c != EOF && p < spell_buffer + 255) {
                    *p++ = c;
                }
                *p = '\0';
                pObjIndex->materia_spell = strdup(spell_buffer);
            } else {
                pObjIndex->materia_spell = strdup("");
            }
            fprintf(stderr, "Read materia spell: '%s'\n", pObjIndex->materia_spell);
            pObjIndex->value[1] = 0; // Not used for materia
            pObjIndex->value[2] = fread_number(fp);
            pObjIndex->value[3] = fread_number(fp);
            pObjIndex->value[4] = fread_number(fp);
        } else if (pObjIndex->item_type == 5) { // ITEM_WEAPON
            // Read weapon type as string and convert to number
            char *weapon_type_str = fread_word(fp);
            int weapon_type_num = weapon_type_lookup(weapon_type_str);
            pObjIndex->value[0] = weapon_type_num; // Store weapon type as number
            free(weapon_type_str);
            // Read dice values as numbers
            pObjIndex->value[1] = fread_number(fp); // number_of_dice
            pObjIndex->value[2] = fread_number(fp); // type_of_dice
            // Read damage type as string
            char *damage_type_str = fread_word(fp);
            pObjIndex->damage_type = strdup(damage_type_str);
            free(damage_type_str);
            // Read weapon flags as string
            char *weapon_flags_str = fread_word(fp);
            pObjIndex->weapon_flags = strdup(weapon_flags_str);
            free(weapon_flags_str);
            // Set unused values
            pObjIndex->value[3] = 0; // Not used for weapon
            pObjIndex->value[4] = 0; // Not used for weapon
        } else if (pObjIndex->item_type == 9) { // ITEM_ARMOR
            // Read armor values as flags (they are stored as flag strings like "CDE")
            pObjIndex->value[0] = fread_flag(fp); // ac_pierce
            pObjIndex->value[1] = fread_flag(fp); // ac_bash
            pObjIndex->value[2] = fread_flag(fp); // ac_slash
            pObjIndex->value[3] = fread_flag(fp); // ac_exotic
            pObjIndex->value[4] = fread_flag(fp); // unused
        } else {
            pObjIndex->value[0] = fread_flag(fp);
            pObjIndex->value[1] = fread_flag(fp);
            pObjIndex->value[2] = fread_flag(fp);
            pObjIndex->value[3] = fread_flag(fp);
            pObjIndex->value[4] = fread_flag(fp);
        }
        fprintf(stderr, "Read values: [%d, %d, %d, %d, %d]\n", 
               pObjIndex->value[0], pObjIndex->value[1], pObjIndex->value[2], 
               pObjIndex->value[3], pObjIndex->value[4]);

        pObjIndex->level = fread_number(fp);
        fprintf(stderr, "Read level: %d\n", pObjIndex->level);
        pObjIndex->weight = fread_number(fp);
        fprintf(stderr, "Read weight: %d\n", pObjIndex->weight);
        pObjIndex->cost = fread_number(fp);
        fprintf(stderr, "Read cost: %d\n", pObjIndex->cost);

        // Read condition
        letter = fread_letter(fp);
        fprintf(stderr, "Read condition letter: '%c'\n", letter);
        switch (letter) {
        case 'P': pObjIndex->condition = 100; break;
        case 'G': pObjIndex->condition = 90; break;
        case 'A': pObjIndex->condition = 75; break;
        case 'W': pObjIndex->condition = 50; break;
        case 'D': pObjIndex->condition = 25; break;
        case 'B': pObjIndex->condition = 10; break;
        case 'R': pObjIndex->condition = 0; break;
        default: pObjIndex->condition = 100; break;
        }

        // Initialize pointers
        pObjIndex->affected = NULL;
        pObjIndex->affected2 = NULL;
        pObjIndex->extra_descr = NULL;
        pObjIndex->affects_out = NULL; // Initialize the new field

        // Read affects and extra descriptions (EXACT MUD LOGIC)
        AFFECT_OUT *affects_head = NULL, *affects_tail = NULL;
        for (;;) {
            letter = fread_letter(fp);
            fprintf(stderr, "Affect loop: got letter '%c'\n", letter);
            
            if (letter == 'A') {
                int loc = fread_number(fp);
                int mod = fread_number(fp);
                fprintf(stderr, "  Reading affect: location=%d, modifier=%d\n", loc, mod);
                AFFECT_OUT *ao = malloc(sizeof(AFFECT_OUT));
                strcpy(ao->type, "normal");
                strncpy(ao->location, affect_location_name(loc), sizeof(ao->location)-1);
                ao->modifier = mod;
                ao->extra[0] = '\0';
                ao->next = NULL;
                if (!affects_head) affects_head = affects_tail = ao;
                else { affects_tail->next = ao; affects_tail = ao; }
                // Handle spell affects
                if (loc == 26 || loc == 27) {
                    char nletter = fread_letter(fp);
                    if (nletter == 'N') {
                        char *spell_name = fread_string(fp);
                        strncpy(ao->extra, spell_name, sizeof(ao->extra)-1);
                        free(spell_name);
                    } else {
                        ungetc(nletter, fp);
                    }
                }
            } else if (letter == 'F') {
                char fwhere = fread_letter(fp);
                int loc = fread_number(fp);
                int mod = fread_number(fp);
                int bitv = fread_flag(fp);
                fprintf(stderr, "  Reading flag affect: where=%c, location=%d, modifier=%d, bitvector=%d\n", fwhere, loc, mod, bitv);
                
                AFFECT_OUT *ao = malloc(sizeof(AFFECT_OUT));
                strcpy(ao->type, "flag");
                snprintf(ao->location, sizeof(ao->location), "F%c:%s", fwhere, affect_location_name(loc));
                ao->modifier = mod;
                if (fwhere == 'A') {
                    // Affect flag
                    snprintf(ao->extra, sizeof(ao->extra), "affect:%s", affect_bit_name(bitv));
                } else if (fwhere == 'B') {
                    // Affect2 flag
                    snprintf(ao->extra, sizeof(ao->extra), "affect2:%s", affect2_bit_name(bitv));
                } else if (fwhere == 'I') {
                    // Immune flag
                    snprintf(ao->extra, sizeof(ao->extra), "immune:%s", immune_bit_name(bitv));
                } else if (fwhere == 'R') {
                    // Resist flag
                    snprintf(ao->extra, sizeof(ao->extra), "resist:%s", resist_bit_name(bitv));
                } else if (fwhere == 'S') {
                    // Shield flag
                    snprintf(ao->extra, sizeof(ao->extra), "shield:%s", shield_bit_name(bitv));
                } else if (fwhere == 'V') {
                    // Vulnerable flag
                    snprintf(ao->extra, sizeof(ao->extra), "vuln:%s", vuln_bit_name(bitv));
                } else if (fwhere == 'W') {
                    // Weapon flag
                    snprintf(ao->extra, sizeof(ao->extra), "weapon:%s", weapon_bit_name(bitv));
                } else {
                    snprintf(ao->extra, sizeof(ao->extra), "bitvector:%d", bitv);
                }
                ao->next = NULL;
                if (!affects_head) affects_head = affects_tail = ao;
                else { affects_tail->next = ao; affects_tail = ao; }
            } else if (letter == 'E') {
                fprintf(stderr, "  Reading extra description\n");
                EXTRA_DESCR_DATA *ed = malloc(sizeof(EXTRA_DESCR_DATA));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = pObjIndex->extra_descr;
                pObjIndex->extra_descr = ed;
            } else if (letter == 'N') {
                fprintf(stderr, "  Reading spell name\n");
                char *spell_name = fread_string(fp);
                // Could add as a spell affect if needed
                free(spell_name);
            } else if (letter == 'R') {
                fprintf(stderr, "  Reading room affect\n");
                int dummy1 = fread_number(fp);
                int dummy2 = fread_number(fp);
                (void)dummy1; (void)dummy2;
            } else if (letter == 'S') {
                fprintf(stderr, "  Reading shield affect\n");
                int dummy1 = fread_number(fp);
                int dummy2 = fread_number(fp);
                char *dummy3 = fread_word(fp);
                free(dummy3);
                (void)dummy1; (void)dummy2;
            } else if (letter == '#') {
                fprintf(stderr, "  Found next object, breaking\n");
                ungetc(letter, fp);
                break;
            } else if (letter == '0') {
                fprintf(stderr, "  Found end of objects section\n");
                ungetc(letter, fp);
                break;
            } else {
                fprintf(stderr, "  Unknown letter '%c', skipping\n", letter);
                // Skip this line and continue
                fread_to_eol(fp);
            }
        }
        pObjIndex->affects_out = affects_head;

        // Add to list
        pObjIndex->next = object_list;
        object_list = pObjIndex;
    }
}

// Print object as JSON
void print_object_json(OBJ_INDEX_DATA *obj) {
    printf("  {\n");
    printf("    \"vnum\": %ld,\n", obj->vnum);
    printf("    \"name\": \"%s\",\n", obj->name ? obj->name : "");
    printf("    \"type\": \"%s\",\n", item_type_name(obj->item_type));
    printf("    \"level\": %d,\n", obj->level);
    char wear_buf[256];
    bitfield_to_names(obj->wear_flags, wear_flag_table, wear_buf, sizeof(wear_buf));
    printf("    \"wear_flags\": \"%s\",\n", wear_buf);
    char extra_buf[256];
    bitfield_to_names(obj->extra_flags, extra_flag_table, extra_buf, sizeof(extra_buf));
    printf("    \"extra_flags\": \"%s\",\n", extra_buf);
    printf("    \"material\": \"%s\",\n", obj->material ? obj->material : "");
    printf("    \"condition\": %d,\n", obj->condition);
    printf("    \"weight\": %d,\n", obj->weight);
    printf("    \"cost\": %d,\n", obj->cost);
    char *escaped_short = escape_json_string(obj->short_descr);
    char *escaped_desc = escape_json_string(obj->description);
    printf("    \"short_descr\": \"%s\",\n", escaped_short);
    printf("    \"description\": \"%s\",\n", escaped_desc);
    free(escaped_short);
    free(escaped_desc);

    // Affects
    printf("    \"affects\": [\n");
    AFFECT_OUT *ao = obj->affects_out;
    int first = 1;
    while (ao) {
        if (!first) printf(",\n");
        first = 0;
        printf("      {\n");
        printf("        \"type\": \"%s\",\n", ao->type);
        printf("        \"location\": \"%s\",\n", ao->location);
        printf("        \"modifier\": %d", ao->modifier);
        if (ao->extra[0]) {
            char *escaped_extra = escape_json_string(ao->extra);
            printf(", \"extra\": \"%s\"", escaped_extra);
            free(escaped_extra);
        }
        printf("\n      }");
        ao = ao->next;
    }
    printf("\n    ],\n");

    // Values (interpreted per item type)
    printf("    \"values\": {\n");
    if (obj->item_type == 9) { // armor
        printf("      \"ac_pierce\": %d,\n", obj->value[0]);
        printf("      \"ac_bash\": %d,\n", obj->value[1]);
        printf("      \"ac_slash\": %d,\n", obj->value[2]);
        printf("      \"ac_exotic\": %d,\n", obj->value[3]);
        printf("      \"v4\": %d\n", obj->value[4]);
    } else if (obj->item_type == 5) { // weapon
        printf("      \"weapon_type\": \"%s\",\n", weapon_type_name(obj->value[0]));
        printf("      \"number_of_dice\": %d,\n", obj->value[1]);
        printf("      \"type_of_dice\": %d,\n", obj->value[2]);
        printf("      \"damage_type\": \"%s\",\n", obj->damage_type ? obj->damage_type : "unknown");
        char *flags_names = weapon_flags_to_names(obj->weapon_flags);
        printf("      \"flags\": %s\n", flags_names);
        free(flags_names);
    } else if (obj->item_type == 40) { // materia
        printf("      \"charges\": %d,\n", obj->value[0]);
        printf("      \"spell\": \"%s\",\n", obj->materia_spell ? obj->materia_spell : "");
        printf("      \"v2\": %d,\n", obj->value[2]);
        printf("      \"v3\": %d,\n", obj->value[3]);
        printf("      \"v4\": %d\n", obj->value[4]);
    } else if (obj->item_type == 9) { // armor
        printf("      \"ac_pierce\": %d,\n", obj->value[0]);
        printf("      \"ac_bash\": %d,\n", obj->value[1]);
        printf("      \"ac_slash\": %d,\n", obj->value[2]);
        printf("      \"ac_exotic\": %d,\n", obj->value[3]);
        printf("      \"level\": %d\n", obj->value[4]);
    } else {
        printf("      \"v0\": %d,\n", obj->value[0]);
        printf("      \"v1\": %d,\n", obj->value[1]);
        printf("      \"v2\": %d,\n", obj->value[2]);
        printf("      \"v3\": %d,\n", obj->value[3]);
        printf("      \"v4\": %d\n", obj->value[4]);
    }
    printf("    }\n");
    printf("  }");
    
    // Clean up materia spell names
    if (obj->item_type == 40 && obj->materia_spell) {
        free(obj->materia_spell);
    }
    // Clean up weapon fields
    if (obj->item_type == 5) {
        if (obj->damage_type) free(obj->damage_type);
        if (obj->weapon_flags) free(obj->weapon_flags);
    }
    // Clean up extra_flags_str
    if (obj->extra_flags_str) {
        free(obj->extra_flags_str);
    }


}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <area_file>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
        return 1;
    }

    // Initialize current_area
    current_area = malloc(sizeof(AREA_DATA));
    current_area->name = strdup("Unknown");
    current_area->file_name = strdup(argv[1]);
    current_area->credits = strdup("Unknown");
    current_area->builders = strdup("Unknown");

    // EXACT MUD LOGIC - copy from db.c
    for (;;) {
        char *word;
        char letter = fread_letter(fp);
        if (letter != '#') {
            fprintf(stderr, "Error: # not found.\n");
            break;
        }

        word = fread_word(fp);
        fprintf(stderr, "Read section: %s\n", word);

        // Remove leading # if present
        if (word && word[0] == '#') {
            word++;
        }

        if (word[0] == '$')
            break;
        else if (!strcmp(word, "AREA"))
            ; // Skip
        else if (!strcmp(word, "MOBOLD"))
            ; // Skip
        else if (!strcmp(word, "AREADATA")) {
            // Skip area data by reading until next #
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    ungetc(c, fp);
                    break;
                }
            }
        }
        else if (!strcmp(word, "HELPS")) {
            // Skip helps by reading until next #
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    ungetc(c, fp);
                    break;
                }
            }
        }
        else if (!strcmp(word, "MOBILES")) {
            // Skip mobiles by reading until next #0 or section
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    char *next_word = fread_word(fp);
                    if (next_word && (!strcmp(next_word, "0") || !strcmp(next_word, "OBJECTS") || !strcmp(next_word, "ROOMS") || !strcmp(next_word, "RESETS") || !strcmp(next_word, "SHOPS") || !strcmp(next_word, "MOBPROGS") || !strcmp(next_word, "SPECIALS"))) {
                        ungetc(c, fp);
                        free(next_word);
                        break;
                    }
                    if (next_word) free(next_word);
                }
            }
        }
        else if (!strcmp(word, "OBJOLD")) {
            // Skip old objects by reading until next #
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    ungetc(c, fp);
                    break;
                }
            }
        }
        else if (!strcmp(word, "OBJECTS")) {
            fprintf(stderr, "Found OBJECTS section, calling load_objects\n");
            load_objects(fp);
            fprintf(stderr, "load_objects returned\n");
        }
        else if (!strcmp(word, "RESETS")) {
            // Skip resets by reading until next #
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    ungetc(c, fp);
                    break;
                }
            }
        }
        else if (!strcmp(word, "ROOMS")) {
            // Skip rooms by reading until next #
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    ungetc(c, fp);
                    break;
                }
            }
        }
        else if (!strcmp(word, "SHOPS")) {
            // Skip shops by reading until next #
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    ungetc(c, fp);
                    break;
                }
            }
        }
        else if (!strcmp(word, "MOBPROGS")) {
            // Skip mobprogs by reading until next #
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    ungetc(c, fp);
                    break;
                }
            }
        }
        else if (!strcmp(word, "SPECIALS")) {
            // Skip specials by reading until next #
            for (;;) {
                char c = fread_letter(fp);
                if (c == EOF) break;
                if (c == '#') {
                    ungetc(c, fp);
                    break;
                }
            }
        }
        else {
            fprintf(stderr, "Unknown section: %s\n", word);
        }
    }

    fclose(fp);

    // Output JSON
    printf("{\n");
    printf("  \"area\": {\n");
    printf("    \"name\": \"%s\",\n", current_area->name ? current_area->name : "");
    printf("    \"file\": \"%s\",\n", current_area->file_name ? current_area->file_name : "");
    printf("    \"credits\": \"%s\",\n", current_area->credits ? current_area->credits : "");
    printf("    \"builders\": \"%s\"\n", current_area->builders ? current_area->builders : "");
    printf("  },\n");
    printf("  \"objects\": [\n");

    OBJ_INDEX_DATA *obj = object_list;
    bool first = true;
    while (obj) {
        if (!first) printf(",\n");
        first = false;
        print_object_json(obj);
        obj = obj->next;
    }

    printf("\n  ]\n");
    printf("}\n");

    return 0;
} 