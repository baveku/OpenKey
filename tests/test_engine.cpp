#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "../Sources/OpenKey/engine/Engine.h"

// Define the global config variables
int vLanguage = 1;
int vInputType = vTelex;
int vFreeMark = 0;
int vCodeTable = 0; // Unicode
int vSwitchKeyStatus = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 0;
int vQuickTelex = 0;
int vRestoreIfWrongSpelling = 1;
int vFixRecommendBrowser = 0;
int vUseMacro = 0;
int vUseMacroInEnglishMode = 0;
int vAutoCapsMacro = 0;
int vUseSmartSwitchKey = 0;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 0;
int vOtherLanguage = 0;
int vTempOffOpenKey = 0;

Uint16 charToMacKeyCode(char c) {
    switch (tolower(c)) {
        case 'a': return KEY_A;
        case 'b': return KEY_B;
        case 'c': return KEY_C;
        case 'd': return KEY_D;
        case 'e': return KEY_E;
        case 'f': return KEY_F;
        case 'g': return KEY_G;
        case 'h': return KEY_H;
        case 'i': return KEY_I;
        case 'j': return KEY_J;
        case 'k': return KEY_K;
        case 'l': return KEY_L;
        case 'm': return KEY_M;
        case 'n': return KEY_N;
        case 'o': return KEY_O;
        case 'p': return KEY_P;
        case 'q': return KEY_Q;
        case 'r': return KEY_R;
        case 's': return KEY_S;
        case 't': return KEY_T;
        case 'u': return KEY_U;
        case 'v': return KEY_V;
        case 'w': return KEY_W;
        case 'x': return KEY_X;
        case 'y': return KEY_Y;
        case 'z': return KEY_Z;
        case ' ': return KEY_SPACE;
        case '[': return KEY_LEFT_BRACKET;
        case ']': return KEY_RIGHT_BRACKET;
        case '.': return KEY_DOT;
        case ',': return KEY_COMMA;
        case '/':
        case '?': return KEY_SLASH;
        default: return KEY_EMPTY;
    }
}

// Convert unicode 16-bit to UTF-8
std::string utf16ToUtf8(Uint16 ch) {
    std::string out;
    if (ch < 0x80) {
        out += (char)ch;
    } else if (ch < 0x800) {
        out += (char)(0xC0 | (ch >> 6));
        out += (char)(0x80 | (ch & 0x3F));
    } else {
        out += (char)(0xE0 | (ch >> 12));
        out += (char)(0x80 | ((ch >> 6) & 0x3F));
        out += (char)(0x80 | (ch & 0x3F));
    }
    return out;
}

// Convert wide string to UTF-8
std::string u16stringToUtf8(const std::vector<Uint16>& str) {
    std::string out;
    for (auto c : str) {
        out += utf16ToUtf8(c);
    }
    return out;
}

std::string typeString(const std::string& input) {
    vKeyInit();
    std::vector<Uint16> buffer;
    vKeyHookState* pData = (vKeyHookState*)vKeyInit();

    for (char ch : input) {
        Uint16 keyCode = charToMacKeyCode(ch);
        Uint8 capsStatus = (isupper(ch)) ? 1 : 0;
        
        vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, keyCode, capsStatus, false);

        if (pData->code == vDoNothing) {
            // Raw character is passed through
            buffer.push_back((Uint16)ch);
        } else if (pData->code == vWillProcess || pData->code == vRestore || pData->code == vRestoreAndStartNewSession) {
            // Delete backspaceCount characters from buffer
            for (int b = 0; b < pData->backspaceCount && !buffer.empty(); b++) {
                buffer.pop_back();
            }
            // Append newCharCount characters
            for (int n = pData->newCharCount - 1; n >= 0; n--) {
                Uint32 cd = pData->charData[n];
                if (cd & PURE_CHARACTER_MASK) {
                    buffer.push_back((Uint16)(cd & 0xFFFF));
                } else if (!(cd & CHAR_CODE_MASK)) {
                    buffer.push_back(keyCodeToCharacter(cd));
                } else {
                    buffer.push_back((Uint16)(cd & 0xFFFF));
                }
            }
        }
    }
    return u16stringToUtf8(buffer);
}

int main() {
    std::cout << "Running comprehensive OpenKey engine verification..." << std::endl;
    
    struct TestCase {
        std::string input;
        std::string expected;
    };

    std::vector<TestCase> cases = {
        // English words that start with "ex" (the reported issue)
        {"ex", "ex"},
        {"exit", "exit"},
        {"exam", "exam"},
        {"extra", "extra"},
        {"export", "export"},
        {"express", "express"},
        {"excel", "excel"},
        {"exact", "exact"},
        {"exist", "exist"},
        {"extend", "extend"},
        {"expect", "expect"},
        {"expert", "expert"},
        {"example", "example"},

        // Other single vowels + x collisions
        {"ox", "ox"},
        {"oxford", "oxford"},
        {"oxygen", "oxygen"},
        {"oxide", "oxide"},
        {"ux", "ux"},
        {"ax", "ax"},
        {"axis", "axis"},
        {"axiom", "axiom"},

        // Single vowels + r collisions
        {"er", "er"},
        {"error", "error"},
        {"era", "era"},
        {"or", "or"},
        {"order", "order"},
        {"origin", "origin"},
        {"orange", "orange"},

        // English words with -sh, -sk, -sp, -sm (after S) that never conflict with Vietnamese
        {"fast", "fast"},
        {"just", "just"},
        {"wish", "wish"},
        {"fish", "fish"},
        {"dish", "dish"},
        {"push", "push"},
        {"crash", "crash"},
        {"flash", "flash"},
        {"fresh", "fresh"},
        {"task", "task"},
        {"risk", "risk"},
        {"desk", "desk"},
        {"mask", "mask"},
        {"clasp", "clasp"},
        {"prism", "prism"},

        // Vietnamese words typed with acute mark right after vowel (Method 1)
        {"phasp", "pháp"},
        {"thasp", "tháp"},
        {"casp", "cáp"},
        {"nhasp", "nháp"},
        {"tasp", "táp"},
        {"basc", "bác"},
        {"casc", "các"},
        {"rasc", "rác"},
        {"tasc", "tác"},
        {"tasm", "tám"},
        {"casm", "cám"},
        {"dasm", "dám"},
        {"khasm", "khám"},
        {"hast", "hát"},
        {"bast", "bát"},
        {"cast", "cát"},
        {"mast", "mát"},
        {"tast", "tát"},
        {"last", "lát"},
        {"bust", "bút"},
        {"rust", "rút"},
        {"trust", "trút"},
        {"best", "bét"},
        {"rest", "rét"},
        {"test", "tét"},
        {"cost", "cót"},
        {"must", "mút"},
        {"boost", "bốt"},
        {"list", "lít"},

        // English words with -rt, -rk, -rd (after R)
        {"art", "art"},
        {"part", "part"},
        {"start", "start"},
        {"smart", "smart"},
        {"sport", "sport"},
        {"short", "short"},
        {"chart", "chart"},
        {"port", "port"},
        {"report", "report"},
        {"alert", "alert"},
        {"work", "work"},
        {"park", "park"},
        {"dark", "dark"},
        {"mark", "mark"},
        {"fork", "fork"},
        {"card", "card"},
        {"hard", "hard"},
        {"word", "word"},
        {"board", "board"},
        {"record", "record"},
        {"standard", "standard"},
        {"yard", "yard"},
        {"server", "server"},
        {"service", "service"},
        {"version", "version"},
        {"person", "person"},
        {"margin", "margin"},
        {"carbon", "carbon"},
        {"target", "target"},
        {"urgent", "urgent"},
        {"virgin", "virgin"},
        {"bird", "bird"},
        {"third", "third"},
        {"border", "border"},
        {"circle", "circle"},
        {"circus", "circus"},

        // English words with j inside (after J tone mark key)
        {"reject", "reject"},
        {"major", "major"},
        {"enjoy", "enjoy"},
        {"inject", "inject"},
        {"ninja", "ninja"},
        {"trajectory", "trajectory"},
        {"sojourn", "sojourn"},
        {"pajama", "pajama"},
        {"banjo", "banjo"},
        {"deject", "deject"},

        // English words with guest
        {"guest", "guest"},

        // Vietnamese words typed with tone mark after vowel (Telex)
        {"barn", "bản"},
        {"barng", "bảng"},
        {"barnh", "bảnh"},
        {"carm", "cảm"},
        {"tharm", "thảm"},
        {"kieerm", "kiểm"},
        {"carnh", "cảnh"},
        {"quarn", "quản"},
        {"lajp", "lạp"},
        {"lajc", "lạc"},
        {"meejt", "mệt"},
        {"mejt", "mẹt"},
        {"tajm", "tạm"},
        {"bajn", "bạn"},
        {"bajch", "bạch"},
        {"beejnh", "bệnh"},
        {"cari", "cải"},
        {"caaru", "cẩu"},
        {"caaju", "cậu"},
        {"chiju", "chịu"},
        {"guir", "gủi"},
        {"moir", "mỏi"},
        {"bayr", "bảy"},

        // English words with x inside (next, text, taxi, pixel, mixer, etc.)
        {"next", "next"},
        {"text", "text"},
        {"context", "context"},
        {"texture", "texture"},
        {"taxi", "taxi"},
        {"nexus", "nexus"},
        {"sexy", "sexy"},
        {"toxic", "toxic"},
        {"pixel", "pixel"},
        {"mixer", "mixer"},
        {"boxing", "boxing"},
        {"sixteen", "sixteen"},
        {"sixty", "sixty"},
        {"maximum", "maximum"},
        {"flex", "flex"},
        {"flexible", "flexible"},
        {"reflex", "reflex"},
        {"hex", "hex"},

        // English words with -ft, -fl, and prefixes of-, ref-, conf-, sof- (after F)
        {"of", "of"},
        {"off", "off"},
        {"office", "office"},
        {"often", "often"},
        {"offline", "offline"},
        {"offer", "offer"},
        {"official", "official"},
        {"officer", "officer"},
        {"soft", "soft"},
        {"sofa", "sofa"},
        {"software", "software"},
        {"softly", "softly"},
        {"softer", "softer"},
        {"refer", "refer"},
        {"refactor", "refactor"},
        {"refresh", "refresh"},
        {"reflect", "reflect"},
        {"reflex", "reflex"},
        {"reform", "reform"},
        {"refuse", "refuse"},
        {"refine", "refine"},
        {"gift", "gift"},
        {"lift", "lift"},
        {"left", "left"},
        {"craft", "craft"},
        {"after", "after"},
        {"conflict", "conflict"},
        {"deflate", "deflate"},
        {"config", "config"},
        {"confirm", "confirm"},
        {"conference", "conference"},
        {"configure", "configure"},
        {"conform", "conform"},
        {"confidence", "confidence"},
        {"nginx", "nginx"},
        {"nginx.conf", "nginx.conf"},
        {"app.conf", "app.conf"},
        {"git.ref", "git.ref"},
        {"main.dart", "main.dart"},
        {"readme.doc", "readme.doc"},

        // Real Vietnamese words with tone marks on o, e, con + f
        {"sof", "sò"},
        {"ref", "rè"},
        {"conf", "còn"},
        {"refm", "rèm"},
        {"refn", "rèn"},
        {"sofn", "sòn"},
        {"sofng", "sòng"},

        // Vietnamese words with initial consonants + e + x -> ẽ
        {"sex", "sẽ"},
        {"vex", "vẽ"},
        {"lex", "lẽ"},
        {"bex", "bẽ"},
        {"kex", "kẽ"},
        {"khex", "khẽ"},
        {"rex", "rẽ"},
        {"chex", "chẽ"},
        {"trex", "trẽ"},
        {"mex", "mẽ"},
        {"thex", "thẽ"},

        // Vietnamese words with initial consonants + o/u/i/y + x
        {"box", "bõ"},
        {"vox", "võ"},
        {"ngox", "ngõ"},
        {"gox", "gõ"},
        {"rox", "rõ"},
        {"mux", "mũ"},
        {"vux", "vũ"},
        {"lux", "lũ"},
        {"six", "sĩ"},
        {"kyx", "kỹ"},
        {"myx", "mỹ"},
        {"quyx", "quỹ"},

        // Valid standalone/modified vowels with tone/mark in Vietnamese
        {"eox", "ẽo"},
        {"ee", "ê"},
        {"oow", "ơ"},
        {"owr", "ở"},
        {"oor", "ổ"},
        {"uwr", "ử"},
        {"aar", "ẩ"},
        {"awx", "ẵ"},
        {"aax", "ẫ"},
        {"eex", "ễ"},
        {"ar", "ả"},
        {"ur", "ủ"},
        {"ir", "ỉ"},
        {"yr", "ỷ"},
        {"aj", "ạ"},
        {"ej", "ẹ"},

        // Core Vietnamese vocabulary and sentences
        {"toan", "toan"},
        {"toans", "toán"},
        {"nguoi", "nguoi"},
        {"nguoiwf", "người"},
        {"vieetj", "việt"},
        {"nam", "nam"},
        {"tieengs", "tiếng"},
        {"banr", "bản"},
        {"thamr", "thảm"},
        {"canhr", "cảnh"},
        {"bangr", "bảng"},
        {"ddoangf", "đoàng"},
        {"khuyeens", "khuyến"},
        {"nguyeexn", "nguyễn"},
        {"nghieemj", "nghiệm"},

        // Free-mark Vietnamese typing
        {"vongx", "võng"},
        {"voxng", "võng"},
        {"lamx", "lãm"},
        {"laxm", "lãm"},
        {"cungx", "cũng"},
        {"cuxng", "cũng"},
        {"hanhx", "hãnh"},
        {"haxnh", "hãnh"},

        // Uppercase tests
        {"Ex", "Ex"},
        {"Exit", "Exit"},
        {"Exam", "Exam"},
        {"Extra", "Extra"},
        {"Export", "Export"},
        {"Next", "Next"},
        {"Text", "Text"},
        {"Taxi", "Taxi"},
        {"Soft", "Soft"},
        {"Card", "Card"},
        {"Ox", "Ox"},
        {"Oxford", "Oxford"},
        {"Ux", "Ux"},
        {"Ax", "Ax"},
        {"Axis", "Axis"},
        {"Sex", "Sẽ"},
        {"Vex", "Vẽ"},
        {"Bex", "Bẽ"},

        // Sentence tests
        {"Tooi ddi hocj", "Tôi đi học"},
        {"Banj cos khoer khoong?", "Bạn có khỏe không?"},
        {"Tooi caanf ddi exit ngay baay giowf", "Tôi cần đi exit ngay bây giờ"},
        {"Tooi caanf ddi taxi ngay", "Tôi cần đi taxi ngay"},
        {"Next time tows thichs text cho caaju", "Next time tớ thích text cho cậu"},
        {"Nguwoif yeeu ex cuar tooi", "Người yêu ex của tôi"},
        {"Xe ex chayj raats nhanh", "Xe ex chạy rất nhanh"}
    };

    int passed = 0;
    int failed = 0;
    for (const auto& tc : cases) {
        std::string actual = typeString(tc.input);
        if (actual == tc.expected) {
            passed++;
        } else {
            failed++;
            std::cerr << "FAIL: input '" << tc.input << "' expected '" << tc.expected << "' but got '" << actual << "'" << std::endl;
        }
    }
    std::cout << "Results: " << passed << " passed, " << failed << " failed." << std::endl;
    return (failed == 0) ? 0 : 1;
}
