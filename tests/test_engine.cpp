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
        case '1': return KEY_1;
        case '2': return KEY_2;
        case '3': return KEY_3;
        case '4': return KEY_4;
        case '5': return KEY_5;
        case '6': return KEY_6;
        case '7': return KEY_7;
        case '8': return KEY_8;
        case '9': return KEY_9;
        case '0': return KEY_0;
        case ' ': return KEY_SPACE;
        case '[': return KEY_LEFT_BRACKET;
        case ']': return KEY_RIGHT_BRACKET;
        case '.': return KEY_DOT;
        case ',': return KEY_COMMA;
        case '/':
        case '?': return KEY_SLASH;
        case '\b': return KEY_DELETE;
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
            if (keyCode == KEY_DELETE) {
                if (!buffer.empty()) buffer.pop_back();
            } else {
                // Raw character is passed through
                buffer.push_back((Uint16)ch);
            }
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
            // macOS appends the raw key once when the engine restores a mark.
            if (pData->code == vRestore || pData->code == vRestoreAndStartNewSession) {
                buffer.push_back((Uint16)ch);
            }
        } else if (pData->code == vReplaceMaro) {
            for (int b = 0; b < pData->backspaceCount && !buffer.empty(); b++) {
                buffer.pop_back();
            }
            for (size_t n = 0; n < pData->macroData.size(); n++) {
                Uint32 cd = pData->macroData[n];
                if (cd & PURE_CHARACTER_MASK) {
                    buffer.push_back((Uint16)(cd & 0xFFFF));
                } else if (!(cd & CHAR_CODE_MASK)) {
                    buffer.push_back(keyCodeToCharacter(cd));
                } else {
                    buffer.push_back((Uint16)(cd & 0xFFFF));
                }
            }
            buffer.push_back((Uint16)ch);
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

        // Bare "p" + "e" + hỏi is not a Vietnamese syllable ("per" must stay English).
        // "phẻ" uses initial "ph", so it stays a valid Vietnamese word.
        {"per", "per"},
        {"pow", "pow"},
        {"power", "power"},
        {"powder", "powder"},
        {"powell", "powell"},
        {"powerful", "powerful"},
        {"Power", "Power"},
        {"book", "book"},
        {"look", "look"},
        {"took", "took"},
        {"good", "good"},
        {"tool", "tool"},
        {"cool", "cool"},
        {"pool", "pool"},
        {"bool", "bool"},
        {"hawk", "hawk"},
        {"bowl", "bowl"},
        {"howl", "howl"},
        {"owl", "owl"},
        {"school", "school"},
        {"google", "google"},
        {"awesome", "awesome"},
        {"awful", "awful"},
        {"lawyer", "lawyer"},
        {"tower", "tower"},
        {"download", "download"},
        {"owner", "owner"},
        {"choose", "choose"},
        {"async", "async"},
        {"await", "await"},
        {"remember", "remember"},
        {"hello", "hello"},
        {"follow", "follow"},
        {"yellow", "yellow"},
        {"always", "always"},
        {"cookie", "cookie"},
        {"too", "tô"},
        {"moon", "môn"},
        {"been", "bên"},
        {"queen", "quên"},
        {"cow", "cơ"},
        {"own", "ơn"},
        {"lawn", "lăn"},
        {"cooc", "côc"},
        {"toots", "tốt"},
        {"bawc", "băc"},
        {"phow", "phơ"},
        {"mow", "mơ"},
        {"chow", "chơ"},
        {"Per", "Per"},
        {"PER", "PER"},
        {"perm", "perm"},
        {"percent", "percent"},
        {"perfect", "perfect"},
        {"perform", "perform"},
        {"period", "period"},
        {"permission", "permission"},
        {"perhaps", "perhaps"},
        {"pher", "phẻ"},
        {"ber", "bẻ"},
        {"ser", "sẻ"},
        {"ver", "vẻ"},
        {"ler", "lẻ"},
        {"rer", "rẻ"},

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
        {"thichs", "thích"},
        {"thisch", "thích"},
        {"thicsh", "thích"},
        {"icsh", "ích"},
        {"bansh", "bánh"},
        {"chinsh", "chính"},
        {"tieensg", "tiếng"},
        {"tieengs", "tiếng"},
        {"tienegs", "tiếng"},
        {"thayas", "thấy"},
        {"thuajat", "thuật"},
        {"thuaatj", "thuật"},
        {"thuatja", "thuật"},
        {"nhaatj", "nhật"},
        {"xuajat", "xuật"},
        {"luajat", "luật"},
        {"quajat", "quật"},
        {"hoajat", "hoật"},
        {"hoafat", "hoầt"},
        {"viejet", "việt"},
        {"vieest", "viết"},
        {"vietej", "việt"},
        {"tapa", "tapa"},
        {"taapj", "t\u1eadp"},
        {"maapj", "m\u1eadp"},
        {"caapj", "c\u1eadp"},
        {"data", "data"},
        {"tapas", "tapas"},
        {"papa", "papa"},
        {"papas", "papas"},
        {"photo", "photo"},
        {"coco", "coco"},
        {"vietes", "viết"},
        {"bietej", "biệt"},
        {"booj", "b\u1ed9"},
        {"boof", "b\u1ed3"},
        {"boojt", "b\u1ed9t"},
        {"piece", "piece"},
        {"pieces", "pieces"},
        {"thuataj", "thuật"},
        {"tienje", "tiện"},
        {"tienfe", "tiền"},
        {"tiense", "tiến"},
        {"tienre", "tiển"},
        {"tienxe", "tiễn"},
        {"chuyenje", "chuyện"},
        {"there", "there"},
        {"here", "here"},
        {"data", "data"},
        {"sofa", "sofa"},
        {"pajama", "pajama"},
        {"major", "major"},
        {"banjo", "banjo"},
        {"ninja", "ninja"},
        {"swar", "sửa"},
        {"swra", "sửa"},
        {"swfa", "sừa"},
        {"swsa", "sứa"},
        {"swxa", "sữa"},
        {"swja", "sựa"},
        {"xwra", "xửa"},
        {"lwra", "lửa"},
        {"twra", "tửa"},
        {"nwra", "nửa"},
        {"chwra", "chửa"},
        {"swa", "sưa"},
        {"swaf", "sừa"},
        {"swas", "sứa"},
        {"swax", "sữa"},
        {"swaj", "sựa"},
        {"xwa", "xưa"},
        {"lwar", "lửa"},
        {"twar", "tửa"},
        {"nwar", "nửa"},
        {"chwa", "chưa"},
        {"sweet", "sweet"},
        {"swim", "swim"},
        {"switch", "switch"},
        {"answer", "answer"},
        {"thaays", "thấy"},
        {"thayaf", "thầy"},
        {"ngayaf", "ngầy"},
        {"baya", "bây"},
        {"maya", "mây"},
        {"taya", "tây"},
        {"vaya", "vây"},
        {"vayaj", "vậy"},
        {"wwork dods tienegs", "work đó tiếng"},
        {"wwork", "work"},
        {"wwarning", "warning"},
        {"warning", "warning"},
        {"what", "what"},
        {"where", "where"},
        {"who", "who"},
        {"why", "why"},
        {"when", "when"},
        {"which", "which"},
        {"while", "while"},
        {"white", "white"},
        {"toansz", "toan"},
        {"ddosz", "\u0111o"},
        {"vieetjz", "vi\u00eat"},
        {"dods", "đó"},
        {"dudowjc", "được"},
        {"dduowjc", "được"},
        {"duowjc", "dược"},
        {"tienef", "tiền"},
        {"tienex", "tiễn"},
        {"tienej", "tiện"},
        {"thicjh", "thịch"},
        {"banxh", "bãnh"},
        {"hopjw", "hợp"},
        {"hojpw", "hợp"},
        {"howpj", "hợp"},
        {"muotjw", "mượt"},
        {"chanjw", "chặn"},
        {"duocjw", "dược"},
        {"nguoifw", "người"},
        {"phuocjw", "phược"},
        {"lopjw", "lợp"},
        {"truongjw", "trượng"},
        {"answer", "answer"},
        {"unswerving", "unswerving"},
        {"banr", "bản"},
        {"dunss", "duns"},
        {"dunff", "dunf"},
        {"dunrr", "dunr"},
        {"dunjj", "dunj"},
        {"dunxx", "dunx"},
        {"anss", "ans"},
        {"ass", "as"},
        {"aff", "af"},
        {"arr", "ar"},
        {"ajj", "aj"},
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
        {"Tooi lamf per ngay", "Tôi làm per ngay"},
        {"Next time tows thichs text cho caaju", "Next time tớ thích text cho cậu"},
        {"Nguwoif yeeu ex cuar tooi", "Người yêu ex của tôi"},
        {"Xe ex chayj raats nhanh", "Xe ex chạy rất nhanh"},

        // uy rhyme syllables
        {"suyts", "suýt"},
        {"suyst", "suýt"},
        {"buyst", "buýt"},
        {"buyts", "buýt"},
        {"quyst", "quýt"},
        {"quyts", "quýt"},
        {"tuyps", "tuýp"},
        {"tuysp", "tuýp"},
        {"huytj", "huỵt"},
        {"huyjt", "huỵt"},
        {"huyfnh", "huỳnh"},
        {"huynfh", "huỳnh"},
        {"quysnh", "quýnh"},
        {"quynsh", "quýnh"},
        {"huysch", "huých"},
        {"khuyar", "khuỷa"},
        {"khuyra", "khuỷa"},
        {"khuyur", "khuỷu"},
        {"khuyru", "khuỷu"},

        // Diphthong delayed circumflex vs single-vowel English preservation (Pillar 1)
        {"tiemes", "tiếm"},
        {"nghiemej", "nghiệm"},
        {"tieces", "tiếc"},
        {"pieces", "pieces"},
        {"nieces", "nieces"},
        {"sense", "sense"},
        {"dense", "dense"},
        {"tense", "tense"},
        {"theme", "theme"},
        {"themes", "themes"},
        {"meme", "meme"},
        {"scene", "scene"},
        {"bene", "bên"},
        {"mono", "mono"},
        {"mongo", "mongo"},
        {"bongo", "bongo"},
        {"congo", "công"},
        {"manga", "manga"},
        {"mana", "mana"},
        {"banana", "banana"},
        {"canada", "canada"},
        {"teste", "tết"},
        {"tester", "tester"},
        {"bastard", "bastard"},
        {"aspartic", "aspartic"},
        {"afar", "afar"},
        {"ajar", "ajar"},
        {"arachnid", "arachnid"},
        {"bosom", "bosom"},
        {"boron", "boron"},
        {"busy", "busy"},
        {"biform", "biform"},
        {"life", "life"},

        // Delayed d stroke vs English dude
        {"dangd", "đang"},
        {"duowjcd", "được"},
        {"dayd", "đay"},
        {"dosd", "đó"},
        {"danhsd", "đánh"},
        {"dinhjd", "định"},
        {"didnhj", "định"},
        {"didnhj ", "định "},
        {"dodwjt", "đợt"},
        {"dodwjt ", "đợt "},
        {"dadng", "đang"},
        {"dadnh", "đanh"},
        {"dedn", "đen"},
        {"dodng", "đong"},
        {"dudng", "đung"},
        {"dadan", "đân"},
        {"didan", "đian"},
        {"dadc", "đac"},
        {"dadp", "đap"},
        {"dadt", "đat"},
        {"dadm", "đam"},
        {"dodm", "đom"},
        {"dudt", "đut"},
        {"dudc", "đuc"},
        {"dudp", "đup"},
        {"dudm", "đum"},
        {"didm", "đim"},
        {"didt", "đit"},
        {"didch", "đich"},
        {"dado", "đao"},
        {"dadu", "đau"},
        {"dadi", "đai"},
        {"dady", "đay"},
        {"dude", "dude"},

        // Repeated tone keys: cancel mark on 2nd press (keeping ư/ơ/ă/â/ê/ô intact),
        // restore both letters when followed by a vowel (message, current, coffee),
        // Tone cancellation invariant: pressing a tone key a second time cancels the mark and leaves 1 literal char
        // (usser -> user, tassk -> task, casse -> case, tesst -> test, ARrgon -> Argon, Currsor -> Cursor, busns -> buns).
        // Double consonant after a vowel uses 3 presses (messsage -> message, currrent -> current, cofffee -> coffee),
        // matching triple-vowel escape (coool -> cool, seeen -> seen, deeep -> deep).
        {"usser", "user"},
        {"usser ", "user "},
        {"Usser", "User"},
        {"USSER", "USER"},
        {"usse", "use"},
        {"ussing", "using"},
        {"casse", "case"},
        {"basse", "base"},
        {"tesst", "test"},
        {"lisst", "list"},
        {"cosst", "cost"},
        {"losst", "lost"},
        {"hosst", "host"},
        {"sysstem", "system"},
        {"morre", "more"},
        {"corre", "core"},
        {"borre", "bore"},
        {"liffe", "life"},
        {"saffe", "safe"},
        {"sofft", "soft"},
        {"lefft", "left"},
        {"affter", "after"},
        {"gifft", "gift"},
        {"wass", "was"},
        {"warrn", "warn"},
        {"waffer", "wafer"},
        {"dowwn", "down"},
        {"towwn", "town"},
        {"lawwn", "lawn"},
        {"ARrgon", "Argon"},
        {"Currsor", "Cursor"},
        {"currrent", "current"},
        {"tassk", "task"},
        {"tassk ", "task "},
        {"Tassk", "Task"},
        {"Tassk ", "Task "},
        {"TASSK", "TASK"},
        {"TASSK ", "TASK "},
        {"massk", "mask"},
        {"cassk", "cask"},
        {"bassk", "bask"},
        {"dissk", "disk"},
        {"dessk", "desk"},
        {"rissk", "risk"},
        {"dussk", "dusk"},
        {"tuwss", "tưs"},
        {"tawngss", "tăngs"},
        {"messsage", "message"},
        {"sesssion", "session"},
        {"lessson", "lesson"},
        {"isssue", "issue"},
        {"asssign", "assign"},
        {"asssist", "assist"},
        {"possible", "possible"},
        {"password", "password"},
        {"asssert", "assert"},
        {"assset", "asset"},
        {"asssess", "assess"},
        {"express", "express"},
        {"address", "address"},
        {"process", "process"},
        {"success", "success"},
        {"arrray", "array"},
        {"arrrow", "arrow"},
        {"error", "error"},
        {"corrrect", "correct"},
        {"mirrror", "mirror"},
        {"borrrow", "borrow"},
        {"worry", "worry"},
        {"sorrry", "sorry"},
        {"carrry", "carry"},
        {"marrry", "marry"},
        {"cofffee", "coffee"},
        {"offer", "offer"},
        {"office", "office"},
        {"sufffer", "suffer"},
        {"bufffer", "buffer"},
        {"difffer", "differ"},
        {"efffect", "effect"},
        {"afffect", "affect"},
        {"affford", "afford"},
        {"efffort", "effort"},
        {"trafffic", "traffic"},
        {"staff", "staff"},
        {"stuff", "stuff"},
        {"cliff", "cliff"},
        {"stiff", "stiff"},
        {"sheriff", "sheriff"},
        {"tariff", "tariff"},

        // Non-consecutive duplicate tone keys in English words
        {"basis", "basis"},
        {"thesis", "thesis"},
        {"oasis", "oasis"},
        {"crisis", "crisis"},
        {"analysis", "analysis"},
        {"resist", "resist"},
        {"consist", "consist"},
        {"insist", "insist"},
        {"persist", "persist"},
        {"exist", "exist"},
        {"system", "system"},
        {"season", "season"},
        {"disaster", "disaster"},
        {"disease", "disease"},
        {"kansas", "kansas"},

        // Bare initial p- and ge- English words vs Vietnamese
        {"get", "get"},
        {"gene", "gene"},
        {"general", "general"},
        {"pow", "pow"},
        {"power", "power"},
        {"part", "part"},
        {"port", "port"},
        {"porn", "porn"},
        {"past", "past"},
        {"post", "post"},
        {"pest", "pest"},
        {"pure", "pure"},
        {"peer", "peer"},
        {"person", "person"},
        {"percent", "percent"},
        {"perfect", "perfect"},
        {"purpose", "purpose"},
        {"parse", "parse"},
        {"pause", "pause"},
        {"pose", "pose"},
        {"push", "push"},
        {"pins", "pín"},
        {"pacs", "pác"},
        {"ghes", "ghé"},
        {"ghees", "ghế"},
        {"ghej", "ghẹ"},
        {"gheej", "ghệ"},
        {"giaf", "già"},
        {"gias", "giá"},
        {"gieengs", "giếng"},
        {"giengse", "giếng"},

        // S-glide / silent-e English words vs Vietnamese
        {"these", "these"},
        {"lose", "lose"},
        {"chose", "chose"},
        {"those", "those"},
        {"use", "use"},
        {"user", "user"},
        {"case", "case"},
        {"base", "base"},
        {"chase", "chase"},
        {"phase", "phase"},
        {"house", "house"},
        {"mouse", "mouse"},
        {"cause", "cause"},
        {"noise", "noise"},
        {"arise", "arise"},
        {"wise", "wise"},
        {"design", "design"},
        {"desire", "desire"},
        {"result", "result"},
        {"resume", "resume"},
        {"visit", "visit"},
        {"vision", "vision"},
        {"decision", "decision"},
        {"prison", "prison"},

        // Standalone w- initial English words vs Vietnamese ư/ưa/ưng
        {"wrong", "wrong"},
        {"worm", "worm"},
        {"won", "won"},
        {"wizard", "wizard"},
        {"wrap", "wrap"},
        {"way", "way"},
        {"wait", "wait"},
        {"want", "want"},
        {"warm", "warm"},
        {"warn", "warn"},
        {"win", "win"},
        {"wing", "wing"},
        {"wife", "wife"},
        {"we", "we"},
        {"web", "web"},
        {"well", "well"},
        {"w", "ư"},
        {"ws", "ứ"},
        {"wf", "ừ"},
        {"wr", "ử"},
        {"wx", "ữ"},
        {"wj", "ự"},
        {"wa", "ưa"},
        {"was", "ứa"},
        {"waf", "ừa"},
        {"war", "ửa"},
        {"wax", "ữa"},
        {"waj", "ựa"},
        {"wng", "ưng"},
        {"wngs", "ứng"},

        // Multi-vowel + coda & closed diphthong English words
        {"virus", "virus"},
        {"very", "very"},
        {"query", "query"},
        {"series", "series"},
        {"merit", "merit"},
        {"serif", "serif"},
        {"diary", "diary"},
        {"dairy", "dairy"},
        {"hairy", "hairy"},
        {"theory", "theory"},
        {"memory", "memory"},
        {"history", "history"},
        {"victory", "victory"},
        {"factory", "factory"},
        {"forum", "forum"},
        {"chorus", "chorus"},
        {"basic", "basic"},
        {"music", "music"},
        {"using", "using"},
        {"basin", "basin"},
        {"raisin", "raisin"},
        {"various", "various"},
        {"curious", "curious"},
        {"furious", "furious"},
        {"marine", "marine"},
        {"trains", "trains"},
        {"mains", "mains"},
        {"aims", "aims"},
        {"fault", "fault"},
        {"coins", "coins"},
        {"joins", "joins"},
        {"points", "points"},
        {"suits", "suits"},
        {"fruits", "fruits"},
        {"ruins", "ruins"},
        {"soup", "soup"},
        {"soups", "soups"},
        {"group", "group"},
        {"groups", "groups"},
        {"our", "our"},
        {"your", "your"},
        {"hour", "hour"},
        {"tour", "tour"},
        {"four", "four"},
        {"course", "course"},
        {"source", "source"},

        // Backspace & space-boundary state synchronization
        {"ta x\b\b\baxi", "taxi"},
        {"ta x\b\bs", "tá"},
        {"tooi\b\bext", "text"},
        {"thichs\b\bch", "thích"},
        {"tas\baxi", "taxi"},
        {"tass\bxi", "taxi"},
        {"Toi da\b\bddang code theme", "Toi đang code theme"},
        {"them \b\bme", "theme"},

        // Pillar 2: Unified Telex Cancel (non-adjacent mark/tone/stroke cancel across consonants & trailing space)
        {"busns", "buns"},
        {"busns ", "buns "},
        {"bufnf", "bunf"},
        {"barnr", "banr"},
        {"buxnx", "bunx"},
        {"bajnj", "banj"},
        {"thischs", "thichs"},
        {"tasns", "tans"},
        {"theeme", "theme"},
        {"theeme ", "theme "},
        {"toono", "tono"},
        {"taana", "tana"},
        {"ddand", "dand"},
        {"ddand ", "dand "},
        {"tuwnw", "tunw"},
        {"tuwnw ", "tunw "},
        {"tuwss ", "tưs "},
        {"tawngss ", "tăngs "},
        {"theemss", "thêms"},
        {"theemss ", "thêms "},
        {"deeper", "deeper"},
        {"keeper", "keeper"},
        {"window", "window"},
        {"owner", "owner"},

        // Triple-vowel Telex escape (ooo -> oo, eee -> ee, aaa -> aa)
        {"coool", "cool"},
        {"coool ", "cool "},
        {"toool", "tool"},
        {"toool ", "tool "},
        {"goood", "good"},
        {"boook", "book"},
        {"rooom", "room"},
        {"rooom ", "room "},
        {"mooon", "moon"},
        {"sooon", "soon"},
        {"looop", "loop"},
        {"seeen", "seen"},
        {"deeep", "deep"},
        {"keeep", "keep"},
        {"meeet", "meet"},

        {"poor ", "poor "},
        {"seeing", "seeing"},
        {"booing", "booing"},

        // Toneless Vietnamese syllables with stop codas (p, t, c) preserved on Space
        {"thaat", "thât"},
        {"thaat ", "thât "},
        {"thata", "thât"},
        {"thata ", "thât "},
        {"thatas", "thất"},
        {"thataj", "thật"},
        {"thatsa", "thất"},
        {"thatja", "thật"},
        {"ddep ", "đep "},
        {"duowc ", "dươc "},
        {"nuowc ", "nươc "},
        {"truowc ", "trươc "},
        {"toot ", "tôt "},
        {"moot ", "môt "},
        {"teet ", "têt "},

        // Delayed circumflex across all valid Vietnamese codas (including toneless)
        {"nhata", "nhât"},
        {"bata", "bât"},
        {"tata", "tât"},
        {"mata", "mât"},
        {"lata", "lât"},
        {"capa", "câp"},
        {"mapa", "mâp"},
        {"viete", "viêt"},
        {"thuata", "thuât"},
        {"tete", "têt"},
        {"tepe", "têp"},
        {"toto", "tôt"},
        {"boto", "bôt"},
        {"moto", "môt"},
        {"hopo", "hôp"},
        {"nene", "nên"},
        {"nene ", "nên "},
        {"lene", "lên"},
        {"bene", "bên"},
        {"tene", "tên"},
        {"mene", "mên"},
        {"dene", "dên"},
        {"hene", "hên"},
        {"kene", "kên"},
        {"quene", "quên"},
        {"nense", "nến"},
        {"nenfe", "nền"},
        {"kenhe", "kênh"},
        {"benhe", "bênh"},
        {"lenhe", "lênh"},
        {"tenhe", "tênh"},
        {"lenhse", "lếnh"},
        {"benhje", "bệnh"},
        {"leche", "lêch"},
        {"lechse", "lếch"},
        {"lechje", "lệch"},
        {"xongo", "xông"},
        {"congo", "công"},
        {"dongo", "dông"},
        {"kongo", "kông"},
        {"tongso", "tống"},
        {"congjo", "cộng"},
        {"dongfo", "dồng"},
        {"vanga", "vâng"},
        {"vangsa", "vấng"},
        {"cangsa", "cấng"},
        {"tangsa", "tấng"},
        {"tonso", "tốn"},
        {"tansa", "tấn"},
        {"tesme", "tếm"},
        {"temse", "tếm"},
        {"mojto", "một"},
        {"mootj", "một"},
        {"motjo", "một"},
        {"hojpo", "hộp"},
        {"lojco", "lộc"},
        {"bojto", "bột"},
        {"tosto", "tốt"},
        {"totso", "tốt"},
        {"teste", "tết"},
        {"tetse", "tết"},
        {"tespe", "tếp"},
        {"tepse", "tếp"},
        {"tesce", "tếc"},
        {"tecse", "tếc"},
        {"tapaj", "tập"},
        {"tapja", "tập"},
        {"tajap", "tập"},
        {"mapaj", "mập"},
        {"capaj", "cập"},
        {"dataj", "dật"},
        {"cataj", "cật"},
        {"catas", "cất"},
        {"cana", "cân"},
        {"canaf", "cần"},
        {"canaf ", "cần "},
        {"Canaf", "Cần"},
        {"canas", "cấn"},
        {"canar", "cẩn"},
        {"canax", "cẫn"},
        {"canaj", "cận"},
        {"bana", "bân"},
        {"banaf", "bần"},
        {"banas", "bấn"},
        {"banar", "bẩn"},
        {"banax", "bẫn"},
        {"banaj", "bận"},
        {"manaf", "mần"},
        {"manas", "mấn"},
        {"manar", "mẩn"},
        {"manax", "mẫn"},
        {"manaj", "mận"},
        {"nanaf", "nần"},
        {"nanas", "nấn"},
        {"nanaj", "nận"},
        {"mamaf", "mầm"},
        {"mamas", "mấm"},
        {"mamaj", "mậm"},
        {"mangaf", "mầng"},
        {"memef", "mềm"},
        {"themef", "thềm"},
        {"monof", "mồn"},
        {"monos", "mốn"},
        {"mongof", "mồng"},
        {"mongos", "mống"},
        {"bongof", "bồng"},
        {"bongos", "bống"},
        {"bongoj", "bộng"},
        {"locoj", "lộc"},
        {"cocoj", "cộc"},
        {"photoj", "phột"},
        {"nhajat", "nhật"},
        {"bojo", "bộ"},
        {"bofo", "bồ"},
        {"bojot", "bột"},
        {"thefe", "thề"},
        {"leje", "lệ"},

        // English words with E..E / O..O / A..A / st / sp preserved
        {"theme", "theme"},
        {"meme", "meme"},
        {"gene", "gene"},
        {"scene", "scene"},
        {"sense", "sense"},
        {"dense", "dense"},
        {"tense", "tense"},
        {"mono", "mono"},
        {"mana", "mana"},
        {"manga", "manga"},
        {"banana", "banana"},
        {"canada", "canada"},
        {"data", "data"},
        {"tapa", "tapa"},
        {"tapas", "tapas"},
        {"papa", "papa"},
        {"papas", "papas"},
        {"photo", "photo"},
        {"coco", "coco"},
        {"mojo", "mojo"},
        {"bastard", "bastard"},
        {"aspartic", "aspartic"},
        {"tester", "tester"},

        // Shorthand w (ư/ươ) + codas + tones
        {"xwngs", "xứng"},
        {"xwngs ", "xứng "},
        {"xwng", "xưng"},
        {"xwcs", "xức"},
        {"xwts", "xứt"},
        {"swcs", "sức"},
        {"swts", "sứt"},
        {"swng", "sưng"},
        {"swngs", "sứng"},
        {"swngf", "sừng"},
        {"rwngf", "rừng"},
        {"rwcj", "rực"},
        {"rwts", "rứt"},
        {"rwng", "rưng"},
        {"twngs", "tứng"},
        {"twngf", "từng"},
        {"lwng", "lưng"},
        {"nhwng", "nhưng"},
        {"nhwngx", "những"},
        {"chwcs", "chức"},
        {"thwcs", "thức"},
        {"thwcj", "thực"},
        {"ddwngs", "đứng"},
        {"ddwts", "đứt"},
        {"ddwcj", "đực"},
        {"mwcs", "mức"},
        {"nwocs", "nước"},
        {"trwocs", "trước"},
        {"ddwocj", "được"},
        {"xwong", "xương"},
        {"xwongs", "xướng"},
        {"xwongr", "xưởng"},
        {"swong", "sương"},
        {"thwongf", "thường"},
        {"ngwoif", "người"},
        {"mwoif", "mười"},
        {"cwoif", "cười"},
        {"mwonj", "mượn"},
        {"lwotj", "lượt"},
        {"vwotj", "vượt"},
        {"rwouj", "rượu"},
        {"ruwouj", "rượu"},
        {"ruwouj ", "rượu "},
        {"huwou", "hươu"},
        {"huwou ", "hươu "},
        {"buwous", "bướu"},
        {"buwosu", "bướu"},

        // Tone before w before coda
        {"thaswng", "thắng"},
        {"caswt", "cắt"},
        {"baswt", "bắt"},
        {"gajwp", "gặp"},
        {"xuswng", "xứng"},
        {"hoswp", "hớp"},

        // Diphthongs/triphthongs with tone after first vowel
        {"cura", "của"},
        {"musa", "múa"},
        {"mufa", "mùa"},
        {"luja", "lụa"},
        {"quar", "quả"},
        {"quas", "quá"},
        {"quaf", "quà"},
        {"quaj", "quạ"},
        {"hosa", "hóa"},
        {"hofa", "hòa"},
        {"hora", "hỏa"},
        {"hoja", "họa"},
        {"tosan", "toán"},
        {"tofan", "toàn"},
        {"lojai", "loại"},
        {"ddojan", "đoạn"},
        {"phisa", "phía"},
        {"chifa", "chìa"},
        {"bifa", "bìa"},
        {"tira", "tỉa"},
        {"nghixa", "nghĩa"},
        {"ddija", "địa"},
        {"baso", "báo"},
        {"vafo", "vào"},
        {"baro", "bảo"},
        {"baxo", "bão"},
        {"tajo", "tạo"},
        {"keso", "kéo"},
        {"mefo", "mèo"},
        {"kero", "kẻo"},
        {"kexo", "kẽo"},
        {"kejo", "kẹo"},
        {"ddefo", "đèo"},
        {"giso", "gió"},
        {"gifo", "giò"},
        {"giro", "giỏ"},
        {"gixo", "giõ"},
        {"gijo", "giọ"},
        {"thusy", "thúy"},
        {"thury", "thủy"},
        {"hury", "hủy"},
        {"busy", "busy"},
        {"bury", "bury"},
        {"haxy", "hãy"},
        {"daxy", "dãy"},
        {"gaxy", "gãy"},
        {"naxy", "nãy"},
        {"maxi", "mãi"},
        {"laxi", "lãi"},
        {"caxi", "cãi"},
        {"muxi", "mũi"},
        {"loxio", "lỗi"},
        {"moxio", "mỗi"},
        {"maxua", "mẫu"},
        {"pheeux", "phễu"},
        {"pheexu", "phễu"},
        {"peux", "peux"},
        {"huwxu", "hữu"},
        {"taxi", "taxi"},
        {"maximum", "maximum"},
        {"boxing", "boxing"},
        {"tosio", "tối"},
        {"toiso", "tối"},
        {"casua", "cấu"},
        {"causa", "cấu"},
        {"cafua", "cầu"},
        {"caufa", "cầu"},
        {"keesu", "kếu"},
        {"keuse", "kếu"},
        {"thafya", "thầy"},
        {"thasya", "thấy"},
        {"vajya", "vậy"},
        {"chieefu", "chiều"},
        {"nhieefu", "nhiều"},
        {"ddeefu", "đều"},
        {"leefu", "lều"},
        {"ddefe", "đề"},
        {"ddeje", "đệ"},
        {"swan", "swan"},
        {"swamp", "swamp"},
        {"swap", "swap"},
        {"swarm", "swarm"},
        {"sway", "sway"},
        {"giant", "giant"},
        {"diary", "diary"},
        {"theory", "theory"},
        {"video", "video"},

        // Tone mark replacement (changing tone mid-word or at end of word)
        {"xwngrs", "xứng"},
        {"xwngrs ", "xứng "},
        {"xwngsr", "xửng"},
        {"xwngsf", "xừng"},
        {"xwngsx", "xững"},
        {"xwngsj", "xựng"},
        {"xwrsng", "xứng"},
        {"toansf", "toàn"},
        {"toanfs", "toán"},
        {"toansr", "toản"},
        {"toansx", "toãn"},
        {"toansj", "toạn"},
        {"toanrs", "toán"},
        {"vieetsj", "việt"},
        {"vieetjs", "viết"},
        {"vietesj", "việt"},
        {"vietejs", "viết"},
        {"vietsje", "việt"},
        {"vietjse", "viết"},
        {"thaatsj", "thật"},
        {"thaatjs", "thất"},
        {"thatasj", "thật"},
        {"thatajs", "thất"},
        {"thatsja", "thật"},
        {"thatjsa", "thất"},
        {"tasf", "tà"},
        {"tafs", "tá"},
        {"tasx", "tã"},
        {"taxs", "tá"},
        {"tasj", "tạ"},
        {"tajs", "tá"},
        {"xwrs", "xứ"},
        {"xwsr", "xử"},
        {"ddars", "đá"},
        {"cars", "cars"},
        {"bars", "bars"},
        {"stars", "stars"},
        {"wars", "wars"},
        {"hers", "hers"},
        {"ours", "ours"},
        {"yours", "yours"},
        {"hours", "hours"},
        {"tours", "tours"},
        {"doors", "doors"},
        {"floors", "floors"},
        {"beers", "beers"},
        {"peers", "peers"},
        {"cheers", "cheers"},
        {"first", "first"},
        {"thirst", "thirst"},
        {"burst", "burst"},
        {"worst", "worst"},
        {"marsh", "marsh"},
        {"harsh", "harsh"},
        {"surf", "surf"},
        {"turf", "turf"},
        {"serf", "serf"},
        {"surface", "surface"},
        {"chefs", "chefs"},
        {"roofs", "roofs"},
        {"briefs", "briefs"},
        {"afraid", "afraid"},
        {"africa", "africa"},
        {"israel", "israel"},
        {"disrupt", "disrupt"},
        {"transfer", "transfer"},
        {"transform", "transform"},

        // gi- onset syllables across vowel modifiers, glides, codas, and typing orders
        {"giaay", "giây"},
        {"giaays", "giấy"},
        {"giaayf", "giầy"},
        {"giaasy", "giấy"},
        {"giaafy", "giầy"},
        {"giaaus", "giấu"},
        {"giaauf", "giầu"},
        {"giaasu", "giấu"},
        {"giaafu", "giầu"},
        {"giaams", "giấm"},
        {"giaasm", "giấm"},
        {"giawm", "giăm"},
        {"giaajn", "giận"},
        {"giaanj", "giận"},
        {"giaajt", "giật"},
        {"giaatj", "giật"},
        {"giawjt", "giặt"},
        {"giawtj", "giặt"},
        {"giawsc", "giắc"},
        {"giawcs", "giắc"},
        {"giawng", "giăng"},
        {"giasn", "gián"},
        {"giasp", "giáp"},
        {"giasc", "giác"},
        {"giusp", "giúp"},
        {"giuwjt", "giựt"},
        {"giuwng", "giưng"},

        // iê/yê + huyền delayed circumflex before coda/glide, uê delayed circumflex, and gu- codas
        {"tiefen", "tiền"},
        {"chiefeu", "chiều"},
        {"nhiefeu", "nhiều"},
        {"yefen", "yền"},
        {"yefeu", "yều"},
        {"huene", "huên"},
        {"tuenes", "tuến"},
        {"khuenhe", "khuênh"},
        {"khueches", "khuếch"},
        {"dduee", "đuê"},
        {"ddueenhf", "đuềnh"},
        {"dduenhfe", "đuềnh"},
        {"guoost", "guốt"},
        {"guowst", "gướt"},
        {"guyst", "guýt"},

        // Delayed d stroke on valid Vietnamese rhymes and delayed w on ươu
        {"diujd", "địu"},
        {"dieetjd", "điệt"},
        {"dieengsd", "điếng"},
        {"doatjd", "đoạt"},
        {"doaisd", "đoái"},
        {"doaysd", "đoáy"},
        {"duowtjd", "đượt"},
        {"duootjd", "đuột"},
        {"duowmsd", "đướm"},
        {"duowpsd", "đướp"},
        {"duyd", "đuy"},
        {"duynd", "đuyn"},
        {"duytsd", "đuýt"},
        {"duyeetjd", "đuyệt"},
        {"duyeend", "đuyên"},
        {"dueed", "đuê"},
        {"dueenhd", "đuênh"},
        {"huouw", "hươu"},
        {"buouws", "bướu"},
        {"ruouwj", "rượu"},

        // Open-syllable and diphthong tone changes
        {"bafr", "bả"},
        {"basr", "bả"},
        {"barf", "bà"},
        {"befs", "bé"},
        {"bofs", "bó"},
        {"beefs", "bế"},
        {"boofs", "bố"},
        {"beofs", "béo"},
        {"baifr", "bải"},
        {"bieeurs", "biếu"},
        {"buooirs", "buối"},
        {"baausr", "bẩu"},
        {"baaurf", "bầu"},
        {"baaurs", "bấu"},
        {"mwofis", "mưới"},
        {"gwris", "gứi"},
        {"basif", "bài"},
        {"bafis", "bái"},

        // English words with ss/ff/rr + consonant, invalid codas, or cancelled non-adjacent keys
        {"poor", "poor"},
        {"poorly", "poorly"},
        {"poorly ", "poorly "},
        {"onshore", "onshore"},
        {"onshore ", "onshore "},
        {"cofactor", "cofactor"},
        {"cofactor ", "cofactor "},
        {"coracle", "coracle"},
        {"coracle ", "coracle "},
        {"tofu", "tofu"},
        {"renew", "renew"},
        {"beeswax", "beeswax"},
        {"airway", "airway"},
        {"upswing", "upswing"},
        {"amassment", "amassment"},
        {"recesslike", "recesslike"},
        {"gonorrhea", "gonorrhea"},
        {"diarrrhea", "diarrhea"},
        {"cirrrhosis", "cirrhosis"},
        {"hasssle", "hassle"},
        {"missspell", "misspell"},
        {"missstep", "misstep"},
        {"losssless", "lossless"},
        {"chesssboard", "chessboard"},
        {"guessswork", "guesswork"},
        {"bafffle", "baffle"},
        {"rafffle", "raffle"},
        {"wafffle", "waffle"},
        {"shuffle", "shuffle"},
        {"offload", "offload"},
        {"offshore", "offshore"},
        {"offspring", "offspring"},
        {"downward", "downward"},
        {"waxwork", "waxwork"},
        {"veneer", "veneer"},
        {"keener", "keener"},
        {"leeches", "leeches"},
        {"daddy", "daddy"},
        {"dodder", "dodder"},
        {"dude", "dude"},
        {"dudes", "dudes"},
        {"diadem", "diadem"},
        {"dodge", "dodge"},
        {"dodrans", "dodrans"},
        {"duopod", "duopod"},
        {"basque", "basque"},
        {"booze", "booze"},
        {"amaze", "amaze"},
        {"borzoi", "borzoi"},
        {"yes", "yes"},
        {"year", "year"},
        {"tier", "tier"},
        {"truer", "truer"},
        {"buyer", "buyer"},
        {"bowie", "bowie"},
        {"coaxial", "coaxial"},
        {"thorax", "thorax"},
        {"borax", "borax"},
        {"pharos", "pharos"},
        {"rosary", "rosary"},
        {"ghafir", "ghafir"},
        {"khajur", "khajur"},
        {"censer", "censer"},
        {"temser", "temser"},
        {"tonsor", "tonsor"},
        {"khuatw", "khuăt"},
        {"khuatw ", "khuăt "},
        {"hoacw ", "hoăc "},
        {"khueenh ", "khuênh "},
        {"uuww", "uuw"},
        {"oaww", "oaw"},
        {"thuoww", "thuow"}
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

    // Verify VNI mode
    {
        vInputType = vVNI;
        std::vector<TestCase> vniCases = {
            {"them6", "thêm"},
            {"hom6", "hôm"},
            {"mon6", "môn"},
            {"bong6", "bông"},
            {"mong6", "mông"},
            {"can6", "cân"},
            {"ban6", "bân"},
            {"man6", "mân"},
            {"mam6", "mâm"},
            {"tap65", "tập"},
            {"cat65", "cật"},
            {"dat65", "dật"},
            {"coc61", "cốc"},
            {"loc65", "lộc"},
            {"TO6", "TÔ"},
            {"to16", "tố"},
            {"tie1n6", "tiến"},
            {"in1", "ín"},
            {"o2", "ò"},
            {"o3", "ỏ"},
            {"e3", "ẻ"},
            {"e4", "ẽ"},
            {"to4", "tõ"}
        };
        for (const auto& tc : vniCases) {
            std::string actual = typeString(tc.input);
            if (actual == tc.expected) {
                passed++;
            } else {
                failed++;
                std::cerr << "FAIL (VNI): input '" << tc.input << "' expected '" << tc.expected << "' but got '" << actual << "'" << std::endl;
            }
        }
        vInputType = vTelex;
    }

    // Verify Simple Telex 1 & 2
    {
        vInputType = vSimpleTelex1;
        for (const auto& tc : std::vector<TestCase>{
            {"cool", "cool"},
            {"veneer", "veneer"},
            {"fast", "fast"},
            {"soft", "soft"},
            {"next", "next"}
        }) {
            std::string actual = typeString(tc.input);
            if (actual == tc.expected) {
                passed++;
            } else {
                failed++;
                std::cerr << "FAIL (SimpleTelex1): input '" << tc.input << "' expected '" << tc.expected << "' but got '" << actual << "'" << std::endl;
            }
        }
        vInputType = vSimpleTelex2;
        for (const auto& tc : std::vector<TestCase>{
            {"what", "what"},
            {"where", "where"},
            {"win", "win"},
            {"cool", "cool"},
            {"veneer", "veneer"}
        }) {
            std::string actual = typeString(tc.input);
            if (actual == tc.expected) {
                passed++;
            } else {
                failed++;
                std::cerr << "FAIL (SimpleTelex2): input '" << tc.input << "' expected '" << tc.expected << "' but got '" << actual << "'" << std::endl;
            }
        }
        vInputType = vTelex;
    }

    // Verify Macros with Vietnamese composition, cancellation, and English restoration
    {
        vUseMacro = 1;
        addMacro("ư", "ừm");
        addMacro("như", "như là");
        addMacro("tập", "tập thể dục");
        addMacro("tas", "test_tas");
        addMacro("cool", "rất ngầu");
        addMacro("veneer", "lớp gỗ");
        for (const auto& tc : std::vector<TestCase>{
            {"w ", "ừm "},
            {"nhw ", "như là "},
            {"tapaj ", "tập thể dục "},
            {"tass ", "test_tas "},
            {"cool ", "rất ngầu "},
            {"veneer ", "lớp gỗ "}
        }) {
            std::string actual = typeString(tc.input);
            if (actual == tc.expected) {
                passed++;
            } else {
                failed++;
                std::cerr << "FAIL (Macro): input '" << tc.input << "' expected '" << tc.expected << "' but got '" << actual << "'" << std::endl;
            }
        }
        vUseMacro = 0;
    }

    // Pillar 3: Verify 0 synthetic backspaces on plain ASCII words (e.g. theme, number, handle, render, hello, follow, remember)
    std::vector<std::string> zeroBackspaceWords = {
        "theme", "themes", "number", "handle", "render", "hello", "follow", "remember", "meme", "mono", "mana", "manga", "data", "school"
    };
    for (const auto& word : zeroBackspaceWords) {
        vKeyInit();
        vKeyHookState* pData = (vKeyHookState*)vKeyInit();
        int totalBackspaces = 0;
        for (char ch : word) {
            Uint16 keyCode = charToMacKeyCode(ch);
            Uint8 capsStatus = (isupper(ch)) ? 1 : 0;
            vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, keyCode, capsStatus, false);
            if (pData->code != vDoNothing) {
                totalBackspaces += pData->backspaceCount;
            }
        }
        if (totalBackspaces == 0) {
            passed++;
        } else {
            failed++;
            std::cerr << "FAIL (ZeroFakeBackspace): input '" << word << "' emitted " << totalBackspaces << " backspaces (expected 0)" << std::endl;
        }
    }

    // Pillar 4: Verify minimal backspace prefix on restore (cool, tool, book only backspace 1 char 'ô' on the final consonant)
    {
        for (const auto& word : std::vector<std::string>{"cool", "tool", "book"}) {
            vKeyInit();
            vKeyHookState* pData = (vKeyHookState*)vKeyInit();
            for (char ch : word) {
                Uint16 keyCode = charToMacKeyCode(ch);
                Uint8 capsStatus = (isupper(ch)) ? 1 : 0;
                vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, keyCode, capsStatus, false);
            }
            if (pData->code == vWillProcess && pData->backspaceCount == 1 && pData->newCharCount == 3) {
                passed++;
            } else {
                failed++;
                std::cerr << "FAIL (MinimalBackspacePrefix): '" << word << "' emitted backspaceCount="
                          << (int)pData->backspaceCount << " newCharCount=" << (int)pData->newCharCount << std::endl;
            }
        }
    }

    std::cout << "Results: " << passed << " passed, " << failed << " failed." << std::endl;
    return (failed == 0) ? 0 : 1;
}
