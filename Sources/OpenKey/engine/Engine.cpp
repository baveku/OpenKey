//
//  Engine.cpp
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
#include <iostream>
#include <algorithm>
#include "Engine.h"
#include <string.h>
#include <list>
#include "Macro.h"

static vector<Uint8> _charKeyCode = {
    KEY_BACKQUOTE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUALS,
    KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_BACK_SLASH,
    KEY_SEMICOLON, KEY_QUOTE, KEY_COMMA, KEY_DOT, KEY_SLASH
};

static vector<Uint8> _breakCode = {
    KEY_ESC, KEY_TAB, KEY_ENTER, KEY_RETURN, KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP, KEY_COMMA, KEY_DOT,
    KEY_SLASH, KEY_SEMICOLON, KEY_QUOTE, KEY_BACK_SLASH, KEY_MINUS, KEY_EQUALS, KEY_BACKQUOTE, KEY_TAB
#if _WIN32
	, VK_INSERT, VK_HOME, VK_END, VK_DELETE, VK_PRIOR, VK_NEXT, VK_SNAPSHOT, VK_PRINT, VK_SELECT, VK_HELP,
	VK_EXECUTE, VK_NUMLOCK, VK_SCROLL
#endif
};

static vector<Uint8> _macroBreakCode = {
    KEY_RETURN, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_SEMICOLON, KEY_QUOTE, KEY_BACK_SLASH, KEY_MINUS, KEY_EQUALS
};

static Uint16 ProcessingChar[][11] = {
    {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z}, //Telex
    {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0}, //VNI
    {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z}, //Simple Telex 1
    {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z} //Simple Telex 2
};

#define IS_KEY_Z(key) (ProcessingChar[vInputType][10] == key)
#define IS_KEY_D(key) (ProcessingChar[vInputType][9] == key)
#define IS_KEY_W(key) ((vInputType != vVNI) ? ProcessingChar[vInputType][8] == key : \
                                    (vInputType == vVNI ? (ProcessingChar[vInputType][8] == key || ProcessingChar[vInputType][7] == key) : false))
#define IS_KEY_DOUBLE(key) ((vInputType != vVNI) ? (ProcessingChar[vInputType][5] == key || ProcessingChar[vInputType][6] == key || ProcessingChar[vInputType][7] == key) :\
                                        (vInputType == vVNI ? ProcessingChar[vInputType][6] == key : false))
#define IS_KEY_S(key) (ProcessingChar[vInputType][0] == key)
#define IS_KEY_F(key) (ProcessingChar[vInputType][1] == key)
#define IS_KEY_R(key) (ProcessingChar[vInputType][2] == key)
#define IS_KEY_X(key) (ProcessingChar[vInputType][3] == key)
#define IS_KEY_J(key) (ProcessingChar[vInputType][4] == key)

#define IS_MARK_KEY(keyCode) (((vInputType != vVNI) && (keyCode == KEY_S || keyCode == KEY_F || keyCode == KEY_R || keyCode == KEY_J || keyCode == KEY_X)) || \
                                        (vInputType == vVNI && (keyCode == KEY_1 || keyCode == KEY_2 || keyCode == KEY_3 || keyCode == KEY_5 || keyCode == KEY_4)))
#define IS_BRACKET_KEY(key) (key == KEY_LEFT_BRACKET || key == KEY_RIGHT_BRACKET)

#define VSI vowelStartIndex
#define VEI vowelEndIndex
#define VWSM vowelWillSetMark
#define hBPC HookState.backspaceCount
#define hNCC HookState.newCharCount
#define hCode HookState.code
#define hExt HookState.extCode
#define hData HookState.charData
#define GET getCharacterCode
#define hMacroKey HookState.macroKey
#define hMacroData HookState.macroData

//Data to sendback to main program
vKeyHookState HookState;

//private data
/**
 * data structure of each element in TypingWord (Uint64)
 * first 2 byte is character code or key code.
 * bit 16: has caps or not
 * bit 17: has tone ^ or not
 * bit 18: has tone w or not
 * bit 19 - > 23: has mark or not (Sắc, huyền, hỏi, ngã, nặng)
 * bit 24: is standalone key? (w, [, ])
 * bit 25: is character code or keyboard code; 1: character code; 0: keycode
 */
static Uint32 TypingWord[MAX_BUFF];
static Byte _index = 0;
static vector<Uint32> _longWordHelper; //save the word when _index >= MAX_BUFF
static list<vector<Uint32>> _typingStates; //Aug 28th, 2019: typing helper, save long state of Typing word, can go back and modify the word
vector<Uint32> _typingStatesData;

/**
 * Use for restore key if invalid word
 */
static Uint32 KeyStates[MAX_BUFF];
static Byte _stateIndex = 0;

static bool tempDisableKey = false;
static int deferredBackspace = 0;
static int capsElem;
static int key;
static int markElem;
static bool isCorect = false;
static bool isChanged = false;
static Byte vowelCount = 0;
static Byte vowelStartIndex = 0;
static Byte vowelEndIndex = 0;
static Byte vowelWillSetMark = 0;
static int i, ii, iii;
static int j;
static int k, kk;
static int l;
static bool isRestoredW;
static Uint16 keyForAEO;
static bool isCheckedGrammar;
static bool _isCaps = false;
static int _spaceCount = 0; //add: July 30th, 2019
static bool _hasHandledMacro = false; //for macro flag August 9th, 2019
static Byte _upperCaseStatus = 0; //for Write upper case for the first letter; 2: will upper case
static bool _isCharKeyCode;
static vector<Uint32> _specialChar;
static bool _useSpellCheckingBefore;
static bool _hasHandleQuickConsonant;
static bool _willTempOffEngine = false;
static bool _isInsideFileExtensionOrPath = false;
static bool _restoredOnPrevKey = false;

//function prototype
void findAndCalculateVowel(const bool& forGrammar=false);
void insertMark(const Uint32& markMask, const bool& canModifyFlag=true);

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
wstring utf8ToWideString(const string& str) {
    return converter.from_bytes(str.c_str());
}

string wideStringToUtf8(const wstring& str) {
    return converter.to_bytes(str.c_str());
}

void* vKeyInit() {
    _index = 0;
    _stateIndex = 0;
    tempDisableKey = false;
    _useSpellCheckingBefore = vCheckSpelling;
    _typingStatesData.clear();
    _typingStates.clear();
    _longWordHelper.clear();
    _specialChar.clear();
    hMacroKey.clear();
    _spaceCount = 0;
    _hasHandledMacro = false;
    _hasHandleQuickConsonant = false;
    _willTempOffEngine = false;
    _isInsideFileExtensionOrPath = false;
    _restoredOnPrevKey = false;
    return &HookState;
}

bool isWordBreak(const vKeyEvent& event, const vKeyEventState& state, const Uint16& data) {
    if (event == vKeyEvent::Mouse)
        return true;
    for (i = 0; i < _breakCode.size(); i++) {
        if (_breakCode[i] == data) {
            return true;
        }
    }
    return false;
}

bool isMacroBreakCode(const int& data) {
    for (i = 0; i < _macroBreakCode.size(); i++) {
        if (_macroBreakCode[i] == data) {
            return true;
        }
    }
    return false;
}

void setKeyData(const Byte& index, const Uint16& keyCode, const bool& isCaps) {
    if (index < 0 || index >= MAX_BUFF)
        return;
    TypingWord[index] = keyCode | (isCaps ? CAPS_MASK : 0);
}

bool _spellingOK = false;
bool _spellingFlag = false;
bool _spellingVowelOK = false;
Byte _spellingEndIndex = 0;

bool isValidInitialConsonants(const Byte& count) {
    if (count == 0) return true;
    if (count == 1) {
        Uint16 c0 = CHR(0);
        if (vQuickStartConsonant && (c0 == KEY_F || c0 == KEY_J || c0 == KEY_W)) return true;
        // In Vietnamese, words never start with F, J, W, Z and take Vietnamese tone marks
        if (c0 == KEY_F || c0 == KEY_J || c0 == KEY_W || c0 == KEY_Z) return false;
        return true;
    }
    if (count == 2) {
        Uint16 c0 = CHR(0);
        Uint16 c1 = CHR(1);
        if (c0 == KEY_C && c1 == KEY_H) return true; // ch
        if (c0 == KEY_G && c1 == KEY_H) return true; // gh
        if (c0 == KEY_G && c1 == KEY_I) return true; // gi
        if (c0 == KEY_K && c1 == KEY_H) return true; // kh
        if (c0 == KEY_N && c1 == KEY_H) return true; // nh
        if (c0 == KEY_N && c1 == KEY_G) return true; // ng
        if (c0 == KEY_P && c1 == KEY_H) return true; // ph
        if (c0 == KEY_T && c1 == KEY_H) return true; // th
        if (c0 == KEY_T && c1 == KEY_R) return true; // tr
        if (c0 == KEY_Q && c1 == KEY_U) return true; // qu
        return false;
    }
    if (count == 3) {
        if (CHR(0) == KEY_N && CHR(1) == KEY_G && CHR(2) == KEY_H) return true; // ngh
        return false;
    }
    return false;
}

void checkSpelling(const bool& forceCheckVowel=false) {
    _spellingOK = false;
    _spellingVowelOK = true;
    _spellingEndIndex = _index;
    
    if (_index > 0 && CHR(_index-1) == KEY_RIGHT_BRACKET) {
        _spellingEndIndex = _index-1;
    }
    
    if (_spellingEndIndex > 0) {
        j = 0;
        //Check first consonant
        if (IS_CONSONANT(CHR(0))) {
            while (j < _spellingEndIndex && IS_CONSONANT(CHR(j))) {
                j++;
            }
            // Special case for initial "gi": when followed by a vowel, "gi" acts as initial consonant (length 2)
            if (j == 1 && CHR(0) == KEY_G && _spellingEndIndex > 2 && CHR(1) == KEY_I && !IS_CONSONANT(CHR(2))) {
                j = 2;
            }
            if (!isValidInitialConsonants(j)) {
                _spellingOK = (vAllowConsonantZFWJ || CHR(0) == KEY_P) ? true : false;
                tempDisableKey = true;
                return;
            }
        }
        
        if (j == _spellingEndIndex){ //for "d" case
            _spellingOK = true;
        }
        
        //check next vowel
        k = j;
        VSI = k;
        //August 23rd, 2019: fix case "que't"
        if (CHR(VSI) == KEY_U && k > 0 && k < _spellingEndIndex-1 && CHR(VSI-1) == KEY_Q) {
            k = k + 1;
            j = k;
            VSI = k;
        } else if (_index >= 2 && CHR(0) == KEY_G && CHR(1) == KEY_I && IS_CONSONANT(CHR(2))) {
            VSI = k = j = 1; //Sep 28th: fix gìn
        }
        for (l = 0; l < 3; l++) {
            if (k < _spellingEndIndex && !IS_CONSONANT(CHR(k))) {
                k++;
                VEI = k;
            }
        }
        if (k > j) { //has vowel,
            _spellingVowelOK = false;
            //check correct combined vowel
            if (k - j > 1 && forceCheckVowel) {
                vector<vector<Uint32>>& vowelSet = _vowelCombine[CHR(j)];
                for (l = 0; l < vowelSet.size(); l++) {
                    if (vowelSet[l].size() - 1 != k - j)
                        continue;
                    _spellingFlag = false;
                    for (ii = 1; ii < vowelSet[l].size(); ii++) {
                        if (j + ii - 1 < _spellingEndIndex && vowelSet[l][ii] != ((CHR(j + ii - 1) | (TypingWord[j + ii - 1] & TONEW_MASK) | (TypingWord[j + ii - 1] & TONE_MASK)))) {
                            _spellingFlag = true;
                            break;
                        }
                    }
                    if (_spellingFlag || (k < _spellingEndIndex && !vowelSet[l][0]) || (j + ii - 1 < _spellingEndIndex && !IS_CONSONANT(CHR(j + ii - 1))))
                        continue;
                    
                    _spellingVowelOK = true;
                    break;
                }
                if (_spellingVowelOK && k - j == 2 && CHR(j) == KEY_U && !(TypingWord[j] & TONEW_MASK)) {
                    if (CHR(j + 1) == KEY_O && !(TypingWord[j + 1] & (TONE_MASK | TONEW_MASK))) {
                        _spellingVowelOK = false;
                    } else if (CHR(j + 1) == KEY_A && !(TypingWord[j + 1] & (TONE_MASK | TONEW_MASK)) && k < _spellingEndIndex) {
                        _spellingVowelOK = false;
                    }
                }
            } else if (!IS_CONSONANT(CHR(j))) {
                _spellingVowelOK = true;
            }
            
            //continue check last consonant
            int endConsCount = _spellingEndIndex - k;
            if (endConsCount == 0) {
                _spellingOK = true;
            } else if (endConsCount == 1) {
                Uint16 ec = CHR(k);
                if (ec == KEY_C || ec == KEY_M || ec == KEY_N || ec == KEY_P || ec == KEY_T) {
                    _spellingOK = true;
                } else if (vQuickEndConsonant && (ec == KEY_G || ec == KEY_K || ec == KEY_H)) {
                    _spellingOK = true;
                }
            } else if (endConsCount == 2) {
                Uint16 ec0 = CHR(k), ec1 = CHR(k+1);
                if (ec0 == KEY_N && ec1 == KEY_G) {
                    _spellingOK = true;
                } else if ((ec0 == KEY_C || ec0 == KEY_N) && ec1 == KEY_H) {
                    if (forceCheckVowel && k > j) {
                        Uint16 lastV = CHR(k - 1);
                        Uint32 lastVTone = TypingWord[k - 1] & (TONE_MASK | TONEW_MASK);
                        if ((lastV == KEY_A && lastVTone == 0) ||
                            (lastV == KEY_E && lastVTone == TONE_MASK) ||
                            lastV == KEY_I || lastV == KEY_Y) {
                            _spellingOK = true;
                        }
                    } else {
                        _spellingOK = true;
                    }
                }
            }
            
            //limit: end consonant "c", "ch", "p", "t" can not use with "~", "`", "?"
            if (_spellingOK) {
                Uint32 nucleusMark = 0;
                for (int vi = j; vi < k; vi++) {
                    nucleusMark |= (TypingWord[vi] & MARK_MASK);
                }
                if (_index >= 3 && CHR(_index-1) == KEY_H && CHR(_index-2) == KEY_C && !((nucleusMark & MARK1_MASK) || (nucleusMark & MARK5_MASK) || !nucleusMark)) {
                    _spellingOK = false;
                } else if (_index >= 2 && (CHR(_index-1) == KEY_T || CHR(_index-1) == KEY_C || CHR(_index-1) == KEY_P) && !((nucleusMark & MARK1_MASK) || (nucleusMark & MARK5_MASK) || !nucleusMark)) {
                    _spellingOK = false;
                }
            }
        }
    } else {
        _spellingOK = true;
    }
    tempDisableKey = !(_spellingOK && _spellingVowelOK);
    
    //cout<<"spelling vowel: "<<(_spellingVowelOK ? "OK": "Err")<<endl;
    //cout<<"spelling: "<<(_spellingOK ? "OK": "Err")<<endl<<endl;
}

void checkGrammar(const int& deltaBackSpace) {
    if (_index <= 1 || _index >= MAX_BUFF)
        return;
    
    findAndCalculateVowel(true);
    if (vowelCount == 0)
        return;
    
    isCheckedGrammar = false;
    
    l = VSI;
    
    //if N/C/I/M/P/T/U key for case: "thuơn", "ưoi", "ưom", "ưoc", "ưou"
    if (_index >= 3) {
        for (i = _index-1; i >= 0; i--) {
            if (CHR(i) == KEY_N || CHR(i) == KEY_C || CHR(i) == KEY_I ||
                CHR(i) == KEY_M || CHR(i) == KEY_P || CHR(i) == KEY_T || CHR(i) == KEY_U) {
                if (i - 2 >= 0 && CHR(i - 1) == KEY_O && CHR(i - 2) == KEY_U &&
                    !(i - 2 == 0 && (TypingWord[0] & STANDALONE_MASK))) {
                    if ((TypingWord[i-1] & TONEW_MASK) ^ (TypingWord[i-2] & TONEW_MASK)) {
                        TypingWord[i - 2] |= TONEW_MASK;
                        TypingWord[i - 1] |= TONEW_MASK;
                        isCheckedGrammar = true;
                        break;
                    }
                }
            }
        }
    }
    
    //check mark
    if (_index >= 2) {
        for (i = l; i <= VEI; i++) {
            if (TypingWord[i] & MARK_MASK) {
                Uint32 mark = TypingWord[i] & MARK_MASK;
                TypingWord[i] &= ~MARK_MASK;
                insertMark(mark, false);
                if (i != vowelWillSetMark)
                    isCheckedGrammar = true;
                break;
            }
        }
    }
    
    //re-arrange data to sendback
    if (isCheckedGrammar) {
        if (hCode ==vDoNothing)
            hCode = vWillProcess;
        hBPC = 0;
        
        for (i = _index - 1; i >= l; i--) {
            hBPC++;
            hData[_index - 1 - i] = GET(TypingWord[i]);
        }
        hNCC = hBPC;
        hBPC += deltaBackSpace;
        hExt = 4;
    }
}

void insertKey(const Uint16& keyCode, const bool& isCaps, const bool& isCheckSpelling=true) {
    if (_index >= MAX_BUFF) {
        _longWordHelper.push_back(TypingWord[0]); //save long word
        //left shift
        for (iii = 0; iii < MAX_BUFF - 1; iii++) {
            TypingWord[iii] = TypingWord[iii + 1];
        }
        setKeyData(_index-1, keyCode, isCaps);
    } else {
        setKeyData(_index++, keyCode, isCaps);
    }
    
    if (vCheckSpelling && isCheckSpelling)
        checkSpelling();
    
    //allow d after consonant
    if (keyCode == KEY_D && _index - 2 >= 0 && IS_CONSONANT(CHR(_index - 2)))
        tempDisableKey = false;
}

void insertState(const Uint16& keyCode, const bool& isCaps) {
    if (_stateIndex >= MAX_BUFF) {
        //left shift
        for (iii = 0; iii < MAX_BUFF - 1; iii++) {
            KeyStates[iii] = KeyStates[iii + 1];
        }
        KeyStates[_stateIndex-1] = keyCode | (isCaps ? CAPS_MASK : 0);
    } else {
        KeyStates[_stateIndex++] = keyCode | (isCaps ? CAPS_MASK : 0);
    }
}

void syncKeyStatesFromTypingWord() {
    _stateIndex = 0;
    for (int si = 0; si < _index && _stateIndex < MAX_BUFF; si++) {
        Uint32 w = TypingWord[si];
        Uint16 k = w & CHAR_MASK;
        Uint32 caps = w & CAPS_MASK;
        if ((w & TONEW_MASK) && (w & STANDALONE_MASK) && k == KEY_U) {
            KeyStates[_stateIndex++] = KEY_W | caps;
        } else {
            KeyStates[_stateIndex++] = k | caps;
            if ((w & TONE_MASK) && _stateIndex < MAX_BUFF) {
                KeyStates[_stateIndex++] = k;
            } else if ((w & TONEW_MASK) && !(w & STANDALONE_MASK) && _stateIndex < MAX_BUFF) {
                if (!(si > 0 && (TypingWord[si - 1] & CHAR_MASK) == KEY_U && (TypingWord[si - 1] & TONEW_MASK) && k == KEY_O)) {
                    KeyStates[_stateIndex++] = KEY_W;
                }
            }
        }
        if ((w & MARK_MASK) && _stateIndex < MAX_BUFF) {
            Uint16 markKey = (w & MARK1_MASK) ? KEY_S :
                             (w & MARK2_MASK) ? KEY_F :
                             (w & MARK3_MASK) ? KEY_R :
                             (w & MARK4_MASK) ? KEY_X :
                             (w & MARK5_MASK) ? KEY_J : 0;
            if (markKey != 0) {
                KeyStates[_stateIndex++] = markKey;
            }
        }
    }
}

void saveWord() {
    //save word history
    if (hCode != vReplaceMaro) {
        if (_index > 0) {
            if (_longWordHelper.size() > 0) { //save long word first
                _typingStatesData.clear();
                for (i = 0; i < _longWordHelper.size(); i++) {
                    if (i != 0 && i % MAX_BUFF == 0) { //save if overflow
                        _typingStates.push_back(_typingStatesData);
                        _typingStatesData.clear();
                    }
                    _typingStatesData.push_back(_longWordHelper[i]);
                }
                _typingStates.push_back(_typingStatesData);
                _longWordHelper.clear();
            }
            
            //save current word
            _typingStatesData.clear();
            for (i = 0; i < _index; i++) {
                _typingStatesData.push_back(TypingWord[i]);
            }
            _typingStates.push_back(_typingStatesData);
        }
    } else { //save macro words
        _typingStatesData.clear();
        for (i = 0; i < hMacroData.size(); i++) {
            if (i != 0 && i % MAX_BUFF == 0) { //break if overflow
                _typingStates.push_back(_typingStatesData);
                _typingStatesData.clear();
            }
            _typingStatesData.push_back(hMacroData[i]);
        }
        _typingStates.push_back(_typingStatesData);
    }
}

void saveWord(const Uint32& keyCode, const int& count) {
    _typingStatesData.clear();
    for (i = 0; i < count; i++) {
        _typingStatesData.push_back(keyCode);
    }
    _typingStates.push_back(_typingStatesData);
}

void saveSpecialChar() {
    _typingStatesData.clear();
    for (i = 0; i < _specialChar.size(); i++) {
        _typingStatesData.push_back(_specialChar[i]);
    }
    _typingStates.push_back(_typingStatesData);
    _specialChar.clear();
}

void restoreLastTypingState() {
    if (_typingStates.size() > 0) {
        _typingStatesData = _typingStates.back();
        _typingStates.pop_back();
        if (_typingStatesData.size() > 0){
            if (_typingStatesData[0] == KEY_SPACE) {
                _spaceCount = (int)_typingStatesData.size();
                _index = 0;
            } else if (std::find(_charKeyCode.begin(), _charKeyCode.end(), (Uint16)_typingStatesData[0]) != _charKeyCode.end()) {
                _index = 0;
                _specialChar = _typingStatesData;
                checkSpelling();
            } else {
                for (i = 0; i < _typingStatesData.size(); i++) {
                    TypingWord[i] = _typingStatesData[i];
                }
                _index = (Byte)_typingStatesData.size();
                syncKeyStatesFromTypingWord();
                if (vCheckSpelling)
                    checkSpelling();
            }
        }
    }
}

void startNewSession() {
    _index = 0;
    hBPC = 0;
    hNCC = 0;
    tempDisableKey = false;
    _spellingOK = true;
    _spellingVowelOK = true;
    _stateIndex = 0;
    _hasHandledMacro = false;
    _hasHandleQuickConsonant = false;
    _restoredOnPrevKey = false;
    _longWordHelper.clear();
}

bool canDelayedCircumflex(int targetVowelIdx, Uint16 v) {
    if (targetVowelIdx < 0 || targetVowelIdx >= _index)
        return false;
    if (CHR(targetVowelIdx) != v || (TypingWord[targetVowelIdx] & (TONE_MASK | TONEW_MASK)))
        return false;
    int codaStart = targetVowelIdx + 1;
    int codaLen = _index - codaStart;
    if (codaLen < 1 || codaLen > 2)
        return false;
    Uint16 c0 = CHR(codaStart);
    Uint16 c1 = (codaLen == 2) ? CHR(codaStart + 1) : 0;
    bool validCoda =
        (codaLen == 1 && (c0 == KEY_M || c0 == KEY_N || c0 == KEY_P || c0 == KEY_T || c0 == KEY_C)) ||
        (codaLen == 2 && ((c0 == KEY_N && c1 == KEY_G) ||
                          (v == KEY_E && ((c0 == KEY_N && c1 == KEY_H) || (c0 == KEY_C && c1 == KEY_H)))));
    if (!validCoda)
        return false;

    bool hasMedial = false;
    if (targetVowelIdx >= 1) {
        Uint16 prev = CHR(targetVowelIdx - 1);
        if (v == KEY_E && (prev == KEY_I || prev == KEY_Y || (prev == KEY_U && !(targetVowelIdx == 2 && CHR(0) == KEY_G))) &&
            !(targetVowelIdx - 1 == 1 && CHR(0) == KEY_G && prev == KEY_I)) {
            hasMedial = true;
        } else if (v == KEY_O && prev == KEY_U && !(targetVowelIdx - 1 == 1 && CHR(0) == KEY_Q)) {
            hasMedial = true;
        } else if (v == KEY_A && (prev == KEY_U || prev == KEY_O)) {
            hasMedial = true;
        }
    }

    Uint32 vMark = TypingWord[targetVowelIdx] & MARK_MASK;
    bool hasMark = vMark != 0;

    if (hasMedial) {
        // *quiê- never exists in Vietnamese (quiesce), *iêch/*iênh/*yêch/*yênh never exist, and *uôp never exists (duopod)
        if (v == KEY_E && CHR(targetVowelIdx - 1) == KEY_I && targetVowelIdx == 3 && CHR(0) == KEY_Q && CHR(1) == KEY_U)
            return false;
        if (v == KEY_E && codaLen == 2 && (c0 == KEY_C || c1 == KEY_H) && CHR(targetVowelIdx - 1) != KEY_U)
            return false;
        if (v == KEY_O && codaLen == 1 && c0 == KEY_P)
            return false;
        // piece/pieces, niece/nieces: *piêc and *niêc do not exist in Vietnamese
        if (v == KEY_E && CHR(targetVowelIdx - 1) == KEY_I && codaLen == 1 && c0 == KEY_C &&
            !hasMark && targetVowelIdx == 2 && (CHR(0) == KEY_P || CHR(0) == KEY_N)) {
            return false;
        }
        return true;
    }

    // Single-vowel *êng never exists in Vietnamese (venger, reenge), except after gi- onset (giêng)
    if (v == KEY_E && codaLen == 2 && c0 == KEY_N && c1 == KEY_G &&
        !(targetVowelIdx == 2 && CHR(0) == KEY_G && CHR(1) == KEY_I))
        return false;

    // Single vowel before coda: ensure no other vowel precedes targetVowelIdx
    if (targetVowelIdx >= 1 && !IS_CONSONANT(CHR(targetVowelIdx - 1))) {
        bool isQu = (targetVowelIdx == 2 && CHR(0) == KEY_Q && CHR(1) == KEY_U);
        bool isGi = (targetVowelIdx == 2 && CHR(0) == KEY_G && CHR(1) == KEY_I);
        if (!isQu && !isGi)
            return false;
    }

    bool isStopCoda = (codaLen == 1 && (c0 == KEY_P || c0 == KEY_T || c0 == KEY_C)) ||
                      (codaLen == 2 && c0 == KEY_C && c1 == KEY_H);
    bool isPlainInitialD = (CHR(0) == KEY_D && !(TypingWord[0] & TONE_MASK));

    if (hasMark) {
        if (isStopCoda && !(vMark & (MARK1_MASK | MARK5_MASK)))
            return false;
        // English -sta/-spa/-sma clusters: bastard, aspartic, chasma, phasma
        if (v == KEY_A && codaLen == 1 && (c0 == KEY_T || c0 == KEY_P || (c0 == KEY_M && targetVowelIdx == 2)) &&
            (vMark & MARK1_MASK) && _stateIndex >= 3 &&
            (KeyStates[_stateIndex - 3] & CHAR_MASK) == KEY_S &&
            (KeyStates[_stateIndex - 2] & CHAR_MASK) == c0) {
            return false;
        }
        // English -sche cluster: lesche
        if (v == KEY_E && codaLen == 2 && c0 == KEY_C && c1 == KEY_H && (vMark & MARK1_MASK) &&
            _stateIndex >= 4 && (KeyStates[_stateIndex - 4] & CHAR_MASK) == KEY_S) {
            return false;
        }
        // English -nse words: sense, dense, tense, enserf (*ến does not exist in Vietnamese)
        if (v == KEY_E && codaLen == 1 && c0 == KEY_N && (vMark & MARK1_MASK) &&
            _stateIndex >= 2 && (KeyStates[_stateIndex - 2] & CHAR_MASK) == KEY_S &&
            (targetVowelIdx == 0 || (targetVowelIdx == 1 && (CHR(0) == KEY_S || isPlainInitialD || CHR(0) == KEY_T)))) {
            return false;
        }
        return true;
    }

    // Toneless single-vowel + coda + same vowel:
    // Allowed by default for Vietnamese (thata -> thât, nene -> nên, congo -> công, vanga -> vâng, tete -> têt, toto -> tôt),
    // except for standalone English words of the exact shape Onset + V + Coda + V.
    if (v == KEY_E) {
        // theme, meme, remember/remedy
        if (codaLen == 1 && c0 == KEY_M &&
            ((targetVowelIdx == 2 && CHR(0) == KEY_T && CHR(1) == KEY_H) ||
             (targetVowelIdx == 1 && (CHR(0) == KEY_M || CHR(0) == KEY_R))))
            return false;
        // gene
        if (codaLen == 1 && c0 == KEY_N && targetVowelIdx == 1 && CHR(0) == KEY_G)
            return false;
    } else if (v == KEY_O) {
        // homo
        if (codaLen == 1 && c0 == KEY_M && targetVowelIdx == 1 && CHR(0) == KEY_H)
            return false;
        // mono
        if (codaLen == 1 && c0 == KEY_N && targetVowelIdx == 1 && CHR(0) == KEY_M)
            return false;
        // mongo, bongo
        if (codaLen == 2 && c0 == KEY_N && c1 == KEY_G && targetVowelIdx == 1 &&
            (CHR(0) == KEY_M || CHR(0) == KEY_B))
            return false;
        // photo
        if (codaLen == 1 && c0 == KEY_T && targetVowelIdx == 2 && CHR(0) == KEY_P && CHR(1) == KEY_H)
            return false;
        // coco, loco
        if (codaLen == 1 && c0 == KEY_C && targetVowelIdx == 1 &&
            (CHR(0) == KEY_C || CHR(0) == KEY_L))
            return false;
    } else if (v == KEY_A) {
        // data, cata (catalog/category)
        if (codaLen == 1 && c0 == KEY_T && targetVowelIdx == 1 &&
            (isPlainInitialD || CHR(0) == KEY_C))
            return false;
        // papa, tapa (tapas)
        if (codaLen == 1 && c0 == KEY_P && targetVowelIdx == 1 &&
            (CHR(0) == KEY_P || CHR(0) == KEY_T))
            return false;
        // mama
        if (codaLen == 1 && c0 == KEY_M && targetVowelIdx == 1 && CHR(0) == KEY_M)
            return false;
        // mana, nana, banana, canada
        if (codaLen == 1 && c0 == KEY_N && targetVowelIdx == 1 &&
            (CHR(0) == KEY_M || CHR(0) == KEY_N || CHR(0) == KEY_B || CHR(0) == KEY_C))
            return false;
        // manga
        if (codaLen == 2 && c0 == KEY_N && c1 == KEY_G && targetVowelIdx == 1 && CHR(0) == KEY_M)
            return false;
    }

    return true;
}

void checkCorrectVowel(vector<vector<Uint16>>& charset, int& i, int& k, const Uint16& markKey) {
    //ignore "qu" case
    if (_index >= 2 && CHR(_index-1) == KEY_U && CHR(_index-2) == KEY_Q) {
        isCorect = false;
        return;
    }
    k = _index - 1;
    for (j = (int)charset[i].size() - 1; j >= 0; j--) {
        if ((charset[i][j] & ~(vQuickEndConsonant ? END_CONSONANT_MASK : 0)) != CHR(k)) {
            isCorect = false;
            return;
        }
        k--;
        if (k < 0)
            break;
    }
    
    // Check initial consonants of the word (all consonants before the first vowel)
    int firstVowelIdx = 0;
    while (firstVowelIdx < _index && IS_CONSONANT(CHR(firstVowelIdx))) {
        firstVowelIdx++;
    }
    if (firstVowelIdx == 1 && CHR(0) == KEY_G && _index > 2 && CHR(1) == KEY_I && !IS_CONSONANT(CHR(2))) {
        firstVowelIdx = 2;
    }
    
    // foreign initial consonants bypass vowel combining and mark keys
    if (!isValidInitialConsonants(firstVowelIdx)) {
        isCorect = false;
        return;
    }

    // In Vietnamese orthography, initial "ng" cannot precede 'i', 'e', 'ê' (must be "ngh")
    // Prevents English words like "nginx" from falsely receiving marks ("ngin" + "x" -> "ngĩn")
    if (firstVowelIdx == 2 && CHR(0) == KEY_N && CHR(1) == KEY_G && _index > 2 && (CHR(2) == KEY_I || CHR(2) == KEY_E)) {
        isCorect = false;
        return;
    }

    // Any character between the initial consonants and the matched vowel pattern must be a vowel
    for (int v = firstVowelIdx; v <= k; v++) {
        if (IS_CONSONANT(CHR(v))) {
            isCorect = false;
            return;
        }
    }

    // In Vietnamese, vowel 'e'/'ê' at the start of a vowel nucleus can only be followed by 'o' (eo) or 'u' (êu).
    // Rejects English 'ea', 'ei', 'ey' words (sea, season, seat, read, real, hear, near, year, weird, seize).
    int vowelNucleusStart = (firstVowelIdx == 1 && CHR(0) == KEY_Q && _index > 2 && CHR(1) == KEY_U && !IS_CONSONANT(CHR(2)))
        ? 2 : firstVowelIdx;
    if (vowelNucleusStart + 1 < _index && !IS_CONSONANT(CHR(vowelNucleusStart + 1))) {
        Uint16 v0 = CHR(vowelNucleusStart);
        Uint16 v1 = CHR(vowelNucleusStart + 1);
        bool hasCoda = IS_CONSONANT(CHR(_index - 1));
        if (vowelNucleusStart + 2 < _index && !IS_CONSONANT(CHR(vowelNucleusStart + 2))) {
            Uint16 v2 = CHR(vowelNucleusStart + 2);
            bool validTriphthong =
                ((v0 == KEY_I || v0 == KEY_Y) && v1 == KEY_E && v2 == KEY_U && !hasCoda) ||
                (v0 == KEY_O && ((v1 == KEY_A && (v2 == KEY_I || v2 == KEY_O || v2 == KEY_Y)) ||
                                 (v1 == KEY_E && v2 == KEY_O)) && !hasCoda) ||
                (v0 == KEY_U && ((v1 == KEY_Y && (v2 == KEY_U || v2 == KEY_E || v2 == KEY_A)) ||
                                 (v1 == KEY_O && (v2 == KEY_U || v2 == KEY_I) && !hasCoda) ||
                                 (v1 == KEY_A && (v2 == KEY_Y || v2 == KEY_O) && !hasCoda)));
            if (!validTriphthong) {
                isCorect = false;
                return;
            }
        }
        if (v0 == KEY_E && ((v1 != KEY_O && v1 != KEY_U) || hasCoda)) {
            isCorect = false;
            return;
        }
        // Diphthongs starting with 'a' (ai, ao, au, ay, âu, ây) never take an ending consonant in Vietnamese
        // (rejects main, train, chain, gain, pain, rain, brain, claim, aim, wait, fault, launch, haunt, author).
        if (v0 == KEY_A && (v1 == KEY_E || hasCoda)) {
            isCorect = false;
            return;
        }
        // 'ou' and 'oy' never exist in Vietnamese (rejects house, mouse, our, your, hour, tour, four, course, source, soup, group, boy, toy),
        // and 'oi'/'ôi'/'ơi' never take an ending consonant (rejects coin, join, point, joint).
        if (v0 == KEY_O && (v1 == KEY_U || v1 == KEY_Y || (hasCoda && v1 == KEY_I))) {
            isCorect = false;
            return;
        }
        // 'ui'/'ưi' never take an ending consonant (rejects suit, fruit, ruin, build, built, guilt, guide, juice),
        // 'ia'/'iu' never take an ending consonant, and initial vowel 'y' can only be followed by 'e' (rejects you, your, young).
        if ((v0 == KEY_U && hasCoda && v1 == KEY_I) ||
            (v0 == KEY_I && ((v1 != KEY_A && v1 != KEY_E && v1 != KEY_U) || (hasCoda && v1 != KEY_E))) ||
            (v0 == KEY_Y && v1 != KEY_E)) {
            isCorect = false;
            return;
        }
        if (vInputType != vVNI) {
            // Un-circumflexed 'ye' never takes s/r/x/j in Vietnamese (preserves yes, yer, yex, yest, yesterday).
            if (v0 == KEY_Y && v1 == KEY_E && !(TypingWord[vowelNucleusStart + 1] & TONE_MASK) &&
                (IS_KEY_S(markKey) || IS_KEY_R(markKey) || IS_KEY_X(markKey) || IS_KEY_J(markKey))) {
                isCorect = false;
                return;
            }
            // Open un-circumflexed -ie, -ue, -ye + r is the English -er suffix (tier, bier, truer, buyer, dyer).
            if (!hasCoda && (v0 == KEY_I || v0 == KEY_U || v0 == KEY_Y) &&
                CHR(_index - 1) == KEY_E && !(TypingWord[_index - 1] & TONE_MASK) && IS_KEY_R(markKey)) {
                isCorect = false;
                return;
            }
            // *duop without horn is English duopod (*đuôp/*đuop do not exist in Vietnamese).
            if (IS_KEY_D(markKey) && _index >= 4 && CHR(0) == KEY_D && v0 == KEY_U && v1 == KEY_O &&
                CHR(3) == KEY_P && !(TypingWord[2] & TONEW_MASK)) {
                isCorect = false;
                return;
            }
        }
    }

    // Delayed circumflex (a, e, o) across an ending consonant:
    // Allowed by default for Vietnamese (thata -> thât, nene -> nên, congo -> công, vanga -> vâng),
    // except for standalone English words (theme, meme, gene, sense, dense, tense, piece, niece,
    // mono, mongo, bongo, homo, photo, coco, data, tapa, papa, mama, mana, manga).
    if (vInputType != vVNI && IS_KEY_DOUBLE(markKey) && _index >= 2 && IS_CONSONANT(CHR(_index - 1))) {
        int targetVowelIdx = k + 1;
        if (targetVowelIdx >= 0 && targetVowelIdx < _index && !(TypingWord[targetVowelIdx] & TONE_MASK)) {
            if (!canDelayedCircumflex(targetVowelIdx, CHR(targetVowelIdx))) {
                isCorect = false;
                return;
            }
        }
    }
    
    // Reject acute and tilde marks for standalone 'in'
    if (vInputType != vVNI && firstVowelIdx == 0 && _index >= 2 && CHR(0) == KEY_I && CHR(1) == KEY_N && (IS_KEY_S(markKey) || IS_KEY_X(markKey))) {
        isCorect = false;
        return;
    }
    
    // Reject tilde (x) on standalone single vowels where no Vietnamese word exists
    // (e.g. "ex" -> "ẽ", "ox" -> "õ", "ux" -> "ũ", "ax" -> "ã", "ix" -> "ĩ", "yx" -> "ỹ")
    // Also reject hỏi (r) on standalone unaccented 'e' ("er" -> "ẻ") and 'o' ("or" -> "ỏ")
    // Also reject huyền (f) on standalone unaccented 'o' ("of" -> "ò", because "ò" is not a standalone Vietnamese word)
    if (vInputType != vVNI && firstVowelIdx == 0 && _index == 1 && !(TypingWord[0] & (TONE_MASK | TONEW_MASK))) {
        if (IS_KEY_X(markKey)) {
            isCorect = false;
            return;
        }
        if (IS_KEY_R(markKey) && (CHR(0) == KEY_E || CHR(0) == KEY_O)) {
            isCorect = false;
            return;
        }
        if (IS_KEY_F(markKey) && CHR(0) == KEY_O) {
            isCorect = false;
            return;
        }
    }
    
    // Reject tilde (x) on initial consonant + vowel pairs where no Vietnamese word exists
    // (e.g. "nex" -> "next/nexus", "tex" -> "text/texas", "hex" -> "hex", "tox" -> "toxic/detox")
    if (vInputType != vVNI && firstVowelIdx == 1 && _index == 2 && !(TypingWord[1] & (TONE_MASK | TONEW_MASK)) && IS_KEY_X(markKey)) {
        if ((CHR(1) == KEY_E && (CHR(0) == KEY_N || CHR(0) == KEY_T || CHR(0) == KEY_H)) ||
            (CHR(1) == KEY_O && CHR(0) == KEY_T)) {
            isCorect = false;
            return;
        }
    }

    // In Vietnamese orthography, initial "g" cannot precede 'e' or 'ê' (must be "gh").
    // Prevents English words like "get", "gene", "general" from falsely receiving marks or circumflex.
    if (firstVowelIdx == 1 && CHR(0) == KEY_G && _index >= 2 && CHR(1) == KEY_E) {
        isCorect = false;
        return;
    }

    // Reject tone marks, horn (w), and 'e' circumflex on bare open initial "p".
    // Bare "p" words (per, pow, power, part, port, porn, past, post, pest, pure, peer, poor, pause) stay English,
    // while closed loanwords like "pins" -> "pín" or "pacs" -> "pác" still work.
    if (vInputType != vVNI && firstVowelIdx == 1 && CHR(0) == KEY_P &&
        (IS_KEY_W(markKey) || markKey == KEY_E ||
         (IS_MARK_KEY(markKey) && (CHR(1) == KEY_E || !IS_CONSONANT(CHR(_index - 1)))))) {
        isCorect = false;
        return;
    }
    
    //limit mark for end consonant: "C", "T", "P"
    if (isCorect && charset[i].size() > 1 && (IS_KEY_F(markKey) || IS_KEY_X(markKey) || IS_KEY_R(markKey))) {
        if (charset[i][1] == KEY_C || charset[i][1] == KEY_T || charset[i][1] == KEY_P) {
            isCorect = false;
        } else if (charset[i].size() > 2 && (charset[i][2] == KEY_T || charset[i][2] == KEY_P)) {
            isCorect = false;
        }
    }
    
    if (isCorect && k >= 0) {
        if (CHR(k) == CHR(k+1)) {
            isCorect = false;
        }
    }
}

Uint32 getCharacterCode(const Uint32& data) {
    capsElem = (data & CAPS_MASK) ? 0 : 1;
    key = data & CHAR_MASK;
    if (data & MARK_MASK) { //has mark
        markElem = -2;
        switch (data & MARK_MASK) {
            case MARK1_MASK:
                markElem = 0;
                break;
            case MARK2_MASK:
                markElem = 2;
                break;
            case MARK3_MASK:
                markElem = 4;
                break;
            case MARK4_MASK:
                markElem = 6;
                break;
            case MARK5_MASK:
                markElem = 8;
                break;
        }
        markElem += capsElem;
        
        switch (key) {
            case KEY_A:
            case KEY_O:
            case KEY_U:
            case KEY_E:
                if ((data & TONE_MASK) == 0 && (data & TONEW_MASK) == 0)
                    markElem += 4;
                break;
        }
        
        if (data & TONE_MASK) {
            key |= TONE_MASK;
        } else if (data & TONEW_MASK) {
            key |= TONEW_MASK;
        }
        if (_codeTable[vCodeTable].find(key) == _codeTable[vCodeTable].end())
            return data; //not found
        
        return _codeTable[vCodeTable][key][markElem] | CHAR_CODE_MASK;
    } else { //doesn't has mark
        if (_codeTable[vCodeTable].find(key) == _codeTable[vCodeTable].end())
            return data; //not found
        
        if (data & TONE_MASK) {
            return _codeTable[vCodeTable][key][capsElem] | CHAR_CODE_MASK;
        } else if (data & TONEW_MASK) {
            return _codeTable[vCodeTable][key][capsElem + 2] | CHAR_CODE_MASK;
        } else {
            return data; //not found
        }
    }
    
    return 0;
}

void findAndCalculateVowel(const bool& forGrammar) {
    vowelCount = 0;
    VSI = VEI = 0;
    for (iii = _index - 1; iii >= 0; iii--) {
        if (IS_CONSONANT(CHR(iii))) {
            if (vowelCount > 0)
                break;
        } else {  //is vowel
            if (vowelCount == 0)
                VEI = iii;
            if (!forGrammar) {
                if ((iii-1 >= 0 && (CHR(iii) == KEY_I && CHR(iii-1) == KEY_G)) ||
                    (iii-1 >= 0 && (CHR(iii) == KEY_U && CHR(iii-1) == KEY_Q))) {
                    break;
                }
            }
            VSI = iii;
            vowelCount++;
        }
    }
    //August 26th, 2019: don't count "u" at "q u" as a vowel
    if (VSI - 1 >= 0 && CHR(VSI) == KEY_U && CHR(VSI-1) == KEY_Q) {
        VSI++;
        vowelCount--;
    }
}

void removeMark() {
    findAndCalculateVowel(true);
    isChanged = false;
    if (_index > 0) {
        for (i = VSI; i <= VEI; i++) {
            if (TypingWord[i] & MARK_MASK) {
                TypingWord[i] &= ~MARK_MASK;
                isChanged = true;
            }
        }
    }
    if (isChanged) {
        hCode = vWillProcess;
        hBPC = 0;
        
        for (i = _index - 1; i >= VSI; i--) {
            hBPC++;
            hData[_index - 1 - i] = GET(TypingWord[i]);
        }
        hNCC = hBPC;
    } else {
        hCode = vDoNothing;
    }
}

bool canHasEndConsonant() {
    vector<vector<Uint32>>& vo = _vowelCombine[CHR(VSI)];
    for (ii = 0; ii < vo.size(); ii++) {
        kk = VSI;
        for (iii = 1; iii < vo[ii].size(); iii++) {
            if (kk > VEI || ((CHR(kk) | (TypingWord[kk] & TONE_MASK) | (TypingWord[kk] & TONEW_MASK)) != vo[ii][iii])) {
                break;
            }
            kk++;
        }
        if (iii >= vo[ii].size()) {
            return vo[ii][0] == 1;
        }
    }
    return false;
}

void handleModernMark() {
    //default
    VWSM = VEI;
    hBPC = (_index - VEI);
    
    //rule 2
    if (vowelCount == 3 && ((CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_A && CHR(VSI+2) == KEY_I) ||
                            (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_Y && CHR(VSI+2) == KEY_U) ||
                            (CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_E && CHR(VSI+2) == KEY_O) ||
                            (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_Y && CHR(VSI+2) == KEY_A))) {
        VWSM = VSI + 1;
        hBPC = _index - VWSM;
    } else if ((CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_I) ||
               (CHR(VSI) == KEY_A && CHR(VSI+1) == KEY_I) ||
               (CHR(VSI)== KEY_U && CHR(VSI+1) == KEY_I) ) {
        
        VWSM = VSI;
        hBPC = _index - VWSM;
    } else if (CHR(VEI-1) == KEY_A && CHR(VEI) == KEY_Y) {
        VWSM = VEI - 1;
        hBPC = (_index - VEI) + 1;
    } else if (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_O) {
        VWSM = VSI + 1;
        hBPC = _index - VWSM;
    } else if (CHR(VSI+1) == KEY_O || CHR(VSI+1) == KEY_U) {
        VWSM = VEI - 1;
        hBPC = (_index - VEI) + 1;
    } else if (CHR(VSI) == KEY_O || CHR(VSI) == KEY_U) {
        VWSM = VEI;
        hBPC = (_index - VEI);
    }
    
    //rule 3.1
    if ((CHR(VSI) == KEY_I && CHR(VSI+1) == KEY_E && (TypingWord[VSI+1] & TONE_MASK)) ||
        (CHR(VSI) == KEY_Y && CHR(VSI+1) == KEY_E && (TypingWord[VSI+1] & TONE_MASK)) ||
        (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_O && (TypingWord[VSI+1] & TONE_MASK)) ||
        (CHR(VSI) == KEY_U && (TypingWord[VSI] & TONEW_MASK) && CHR(VSI+1) == KEY_O && (TypingWord[VSI+1] & TONEW_MASK))){
        
        if (VSI+2 < _index) {
            if (CHR(VSI+2) == KEY_P || CHR(VSI+2) == KEY_T ||
                CHR(VSI+2) == KEY_M || CHR(VSI+2) == KEY_N ||
                CHR(VSI+2) == KEY_O || CHR(VSI+2) == KEY_U ||
                CHR(VSI+2) == KEY_I || CHR(VSI+2) == KEY_C ||
                (VSI+3 < _index && CHR(VSI+2) == KEY_C && CHR(VSI+3) == KEY_H) ||
                (VSI+3 < _index && CHR(VSI+2) == KEY_N && CHR(VSI+3) == KEY_H) ||
                (VSI+3 < _index && CHR(VSI+2) == KEY_N && CHR(VSI+3) == KEY_G)) {
                
                VWSM = VSI + 1;
                hBPC = _index - VWSM;
            } else {
                VWSM = VSI;
                hBPC = _index - VWSM;
            }
        } else {
            VWSM = VSI;
            hBPC = _index - VWSM;
        }
    }
    //rule 3.2
    else if ((CHR(VSI) == KEY_I && (CHR(VSI+1) == KEY_A)) ||
             (CHR(VSI) == KEY_Y && (CHR(VSI+1) == KEY_A)) ||
             (CHR(VSI) == KEY_U && (CHR(VSI+1) == KEY_A)) ||
             (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_U && (TypingWord[VSI+1] & TONEW_MASK))){
        
        VWSM = VSI;
        hBPC = _index - VWSM;
    }
    
    //rule 4
    if (vowelCount == 2) {
        if (((CHR(VSI) == KEY_I) && (CHR(VSI+1) == KEY_A)) ||
            ((CHR(VSI) == KEY_I) && (CHR(VSI+1) == KEY_U)) ||
            ((CHR(VSI) == KEY_I) && (CHR(VSI+1) == KEY_O))) {
            
            if (VSI == 0 || (CHR(VSI-1) != KEY_G)) { //dont have G
                VWSM = VSI;
                hBPC = _index - VWSM;
            } else {
                VWSM = VSI + 1;
                hBPC = _index - VWSM;
            }
        } else if ((CHR(VSI) == KEY_U) && (CHR(VSI+1) == KEY_A)) {
            if (VSI == 0 || (CHR(VSI-1) != KEY_Q)) { //dont have Q
                if (VEI + 1 >= _index || !canHasEndConsonant()) {
                    VWSM = VSI;
                    hBPC = _index - VWSM;
                }
            } else {
                VWSM = VSI + 1;
                hBPC = _index - VWSM;
            }
        } else if ((CHR(VSI) == KEY_O) && (CHR(VSI+1) == KEY_O)) { //thoong
            VWSM = VEI;
            hBPC = _index - VWSM;
        }
    }
}

void handleOldMark() {
    //default
    if (vowelCount == 0 && CHR(VEI) == KEY_I)
        VWSM = VEI;
    else
        VWSM = VSI;
    hBPC = (_index - VWSM);
    
    //rule 2
    if (vowelCount == 3 || (VEI + 1 < _index && IS_CONSONANT(CHR(VEI + 1)) && canHasEndConsonant())) {
        VWSM = VSI + 1;
        hBPC = _index - VWSM;
    }
    
    //rule 3
    for (ii = VSI; ii <= VEI; ii++) {
        if ((CHR(ii) == KEY_E && TypingWord[ii] & TONE_MASK) || (CHR(ii) == KEY_O && TypingWord[ii] & TONEW_MASK)) {
            VWSM = ii;
            hBPC = _index - VWSM;
            break;
        }
    }
    
    hNCC = hBPC;
}

void insertMark(const Uint32& markMask, const bool& canModifyFlag) {
    vowelCount = 0;
    
    if (canModifyFlag)
        hCode = vWillProcess;
    hBPC = hNCC = 0;
    
    findAndCalculateVowel();
    VWSM = 0;
    
    //detect mark position
    if (vowelCount == 1) {
        VWSM = VEI;
        hBPC = (_index - VEI);
    } else { //vowel = 2 or 3
        if (vUseModernOrthography == 0)
            handleOldMark();
        else
            handleModernMark();
        if (TypingWord[VEI] & TONE_MASK || TypingWord[VEI] & TONEW_MASK)
            vowelWillSetMark = VEI;
    }
    
    //send data
    kk = _index - 1 - VSI;
    //if duplicate same mark -> restore
    if (TypingWord[VWSM] & markMask) {
        
        TypingWord[VWSM] &= ~MARK_MASK;
        if (canModifyFlag)
            hCode = vRestore;
        for (ii = VSI; ii < _index; ii++) {
            TypingWord[ii] &= ~MARK_MASK;
            hData[kk--] = GET(TypingWord[ii]);
        }
        //_index = 0;
        tempDisableKey = true;
    } else {
        //remove other mark
        TypingWord[VWSM] &= ~MARK_MASK;
        
        //add mark
        TypingWord[VWSM] |= markMask;
        for (ii = VSI; ii < _index; ii++) {
            if (ii != VWSM) { //remove mark for other vowel
                TypingWord[ii] &= ~MARK_MASK;
            }
            hData[kk--] = GET(TypingWord[ii]);
        }
        
        hBPC = _index - VSI;
    }
    hNCC = hBPC;
}

void insertD(const Uint16& data, const bool& isCaps) {
    hCode = vWillProcess;
    hBPC = 0;
    for (ii = _index - 1; ii >= 0; ii--) {
        hBPC++;
        if (CHR(ii) == KEY_D) { //reverse unicode char
            if (TypingWord[ii] & TONE_MASK) {
                //restore and disable temporary
                hCode = vRestore;
                TypingWord[ii] &= ~TONE_MASK;
                hData[_index - 1 - ii] = TypingWord[ii];
                tempDisableKey = true;
                break;
            } else {
                TypingWord[ii] |= TONE_MASK;
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
            }
            break;
        } else { //preresent old char
            hData[_index - 1 - ii] = GET(TypingWord[ii]);
        }
    }
    hNCC = hBPC;
}

void insertAOE(const Uint16& data, const bool& isCaps) {
    findAndCalculateVowel();
    
    //remove W tone
    for (ii = VSI; ii <= VEI; ii++) {
        TypingWord[ii] &= ~TONEW_MASK;
    }
    
    hCode = vWillProcess;
    hBPC = 0;
    
    for (ii = _index - 1; ii >= 0; ii--) {
        hBPC++;
        if (CHR(ii) == data) { //reverse unicode char
            if (TypingWord[ii] & TONE_MASK) {
                //restore and disable temporary
                hCode = vRestore;
                TypingWord[ii] &= ~TONE_MASK;
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
                //_index = 0;
                tempDisableKey = true;
                break;
            } else {
                TypingWord[ii] |= TONE_MASK;
                if (!IS_KEY_D(data))
                    TypingWord[ii] &= ~TONEW_MASK;
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
                
            }
            break;
        } else { //preresent old char
            hData[_index - 1 - ii] = GET(TypingWord[ii]);
        }
    }
    hNCC = hBPC;
}

void insertW(const Uint16& data, const bool& isCaps) {
    isRestoredW = false;
    
    findAndCalculateVowel();
    
    //remove ^ tone
    for (ii = VSI; ii <= VEI; ii++) {
        TypingWord[ii] &= ~TONE_MASK;
    }
    
    if (vowelCount > 1) {
        hBPC = _index - VSI;
        hNCC = hBPC;
        
        if (((TypingWord[VSI] & TONEW_MASK) && (TypingWord[VSI+1] & TONEW_MASK)) ||
            ((TypingWord[VSI] & TONEW_MASK) && (CHR(VSI+1) == KEY_I || CHR(VSI+1) == KEY_A || CHR(VSI+1) == KEY_U)) ||
            (CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_A && (TypingWord[VSI+1] & TONEW_MASK)) ||
            (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_A && (TypingWord[VSI+1] & TONEW_MASK)) ||
            (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_O && (TypingWord[VSI+1] & TONEW_MASK))){
            //restore and disable temporary
            hCode = vRestore;
            
            for (ii = VSI; ii < _index; ii++) {
                TypingWord[ii] &= ~TONEW_MASK;
                hData[_index - 1 - ii] = GET(TypingWord[ii]) & ~STANDALONE_MASK;
            }
            isRestoredW = true;
            tempDisableKey = true;
        } else {
            hCode = vWillProcess;
            
            if ((CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_O)) {
                if (VSI - 2 >= 0 && TypingWord[VSI - 2] == KEY_T && TypingWord[VSI - 1] == KEY_H) {
                    TypingWord[VSI+1] |= TONEW_MASK;
                    if (VSI + 2 < _index && CHR(VSI+2) == KEY_N) {
                        TypingWord[VSI] |= TONEW_MASK;
                    }
                } else if (VSI - 1 >= 0 && TypingWord[VSI - 1] == KEY_Q) {
                    TypingWord[VSI+1] |= TONEW_MASK;
                } else {
                    TypingWord[VSI] |= TONEW_MASK;
                    TypingWord[VSI+1] |= TONEW_MASK;
                }
            } else if (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_A) {
                if (VEI + 1 < _index) {
                    TypingWord[VSI+1] |= TONEW_MASK;
                } else {
                    TypingWord[VSI] |= TONEW_MASK;
                }
            } else if ((CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_I) ||
                       (CHR(VSI) == KEY_U && CHR(VSI+1) == KEY_U) ||
                       (CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_I)) {
                TypingWord[VSI] |= TONEW_MASK;
            } else if ((CHR(VSI) == KEY_I && CHR(VSI+1) == KEY_O) ||
                       (CHR(VSI) == KEY_O && CHR(VSI+1) == KEY_A)) {
                TypingWord[VSI+1] |= TONEW_MASK;
            } else {
                //don't do anything
                tempDisableKey = true;
                isChanged = false;
                hCode = vDoNothing;
            }
            
            for (ii = VSI; ii < _index; ii++) {
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
            }
        }
        
        return;
    }
    
    hCode = vWillProcess;
    hBPC = 0;
    
    for (ii = _index - 1; ii >= 0; ii--) {
        if (ii < VSI)
            break;
        hBPC++;
        switch (CHR(ii)) {
            case KEY_A:
            case KEY_U:
            case KEY_O:
                if (TypingWord[ii] & TONEW_MASK) {
                    //restore and disable temporary
                    if (TypingWord[ii] & STANDALONE_MASK) {
                        hCode = vWillProcess;
                        if (CHR(ii) == KEY_U){
                            TypingWord[ii] = KEY_W | ((TypingWord[ii] & CAPS_MASK) ? CAPS_MASK : 0);
                            // ư + w -> w. The first w is gone from the screen.
                            // Drop it from the key history or wwarning / wwork
                            // print both w's when the word returns to English.
                            if (_stateIndex >= 2
                                && (KeyStates[_stateIndex - 1] & CHAR_MASK) == KEY_W
                                && (KeyStates[_stateIndex - 2] & CHAR_MASK) == KEY_W) {
                                _stateIndex--;
                            }
                        } else if (CHR(ii) == KEY_O) {
                            hCode = vRestore;
                            TypingWord[ii] = KEY_O | ((TypingWord[ii] & CAPS_MASK) ? CAPS_MASK : 0);
                            isRestoredW = true;
                        }
                        hData[_index - 1 - ii] = TypingWord[ii];
                    } else {
                        hCode = vRestore;
                        TypingWord[ii] &= ~TONEW_MASK;
                        hData[_index - 1 - ii] = TypingWord[ii];
                        isRestoredW = true;
                        //_index++;
                    }
                    
                    tempDisableKey = true;
                } else {
                    TypingWord[ii] |= TONEW_MASK;
                    TypingWord[ii] &= ~TONE_MASK;
                    hData[_index - 1 - ii] = GET(TypingWord[ii]);
                }
                break;
                
            default:
                hData[_index - 1 - ii] = GET(TypingWord[ii]);
                break;
        }
    }
    hNCC = hBPC;
    
    if (isRestoredW) {
        //_index = 0;
    }
}

void reverseLastStandaloneChar(const Uint32& keyCode, const bool& isCaps) {
    hCode = vWillProcess;
    hBPC = 0;
    hNCC = 1;
    hExt = 4;
    TypingWord[_index - 1] = (keyCode | TONEW_MASK | STANDALONE_MASK | (isCaps ? CAPS_MASK : 0));
    hData[0] = GET(TypingWord[_index - 1]);
}

void checkForStandaloneChar(const Uint16& data, const bool& isCaps, const Uint32& keyWillReverse) {
    if (CHR(_index - 1) == keyWillReverse && TypingWord[_index - 1] & TONEW_MASK) {
        hCode = vWillProcess;
        hBPC = 1;
        hNCC = 1;
        TypingWord[_index - 1] = data | (isCaps ? CAPS_MASK : 0);
        hData[0] = GET(TypingWord[_index - 1]);
        // ww -> w. The first w is no longer on screen. Drop it from the key
        // history, or a later restore prints both (wwarning, wwork).
        if (_stateIndex >= 2
            && (KeyStates[_stateIndex - 1] & CHAR_MASK) == data
            && (KeyStates[_stateIndex - 2] & CHAR_MASK) == data) {
            _stateIndex--;
        }
        return;
    }
    
    //check standalone w -> ư
    
    if (_index > 0 && CHR(_index-1) == KEY_U && keyWillReverse == KEY_O) {
        insertKey(keyWillReverse, isCaps);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    }
    
    if (_index == 0) { //zero char
        insertKey(data, isCaps, false);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    } else if (_index == 1) { //1 char
        for (i = 0; i < _standaloneWbad.size(); i++) {
            if (CHR(0) == _standaloneWbad[i]) {
                insertKey(data, isCaps);
                return;
            }
        }
        insertKey(data, isCaps, false);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    } else if (_index == 2) {
        for (i = 0; i < _doubleWAllowed.size(); i++) {
            if (CHR(0) == _doubleWAllowed[i][0] && CHR(1) == _doubleWAllowed[i][1]) {
                insertKey(data, isCaps, false);
                reverseLastStandaloneChar(keyWillReverse, isCaps);
                return;
            }
        }
        insertKey(data, isCaps);
        return;
    }
    
    insertKey(data, isCaps);
}

void upperCaseFirstCharacter() {
    if (!(TypingWord[0] & CAPS_MASK)) {
        hCode = vWillProcess;
        hBPC = 0;
        hNCC = 1;
        TypingWord[0] |= CAPS_MASK;
        hData[0] = GET(TypingWord[0]);
        _upperCaseStatus = 0;
        if (vUseMacro && hMacroKey.size() > 0)
            hMacroKey[0] |= CAPS_MASK;
    }
}

bool canKeyWModify() {
    if (vInputType != vSimpleTelex1) {
        if (_index == 0)
            return true;
        if (CHR(_index - 1) == KEY_U && (TypingWord[_index - 1] & TONEW_MASK))
            return true;
        bool allCons = true;
        for (int si = 0; si < _index; si++) {
            if (!IS_CONSONANT(CHR(si)) && !(si == 1 && (CHR(0) == KEY_Q || CHR(0) == KEY_G))) {
                allCons = false;
                break;
            }
        }
        if (allCons && _index <= 2)
            return true;
    }
    int saveI = i, saveJ = j, saveK = k;
    bool saveCorrect = isCorect;
    vector<vector<Uint16>>& charset = _vowel[KEY_W];
    bool found = false;
    for (int idx = 0; idx < (int)charset.size(); idx++) {
        if (_index < charset[idx].size())
            continue;
        isCorect = true;
        int kk = _index;
        checkCorrectVowel(charset, idx, kk, KEY_W);
        if (isCorect) {
            found = true;
            break;
        }
    }
    i = saveI; j = saveJ; k = saveK; isCorect = saveCorrect;
    if (!found)
        return false;
    findAndCalculateVowel();
    if (vowelCount > 1) {
        Uint16 v0 = CHR(VSI), v1 = CHR(VSI + 1);
        return (v0 == KEY_U && (v1 == KEY_O || v1 == KEY_A || v1 == KEY_I || v1 == KEY_U)) ||
               (v0 == KEY_O && (v1 == KEY_I || v1 == KEY_A)) ||
               (v0 == KEY_I && v1 == KEY_O) ||
               ((TypingWord[VSI] & TONEW_MASK) && ((TypingWord[VSI + 1] & TONEW_MASK) || v1 == KEY_I || v1 == KEY_A || v1 == KEY_U));
    }
    return vowelCount == 1;
}

// tapaj -> tập, dataj -> dật, cataj -> cật.
// For tapa/data/cata, the second 'a' stays plain (preserving English tapa/tapas/data/catalog)
// unless followed by nặng (j). All other Vietnamese words (thata -> thât, viete -> viêt, thuata -> thuât)
// convert immediately on the second vowel.
bool deferredCircumflex(bool extraTyped, Uint16 vowel, int& vowelAt) {
    vowelAt = -1;
    if (vInputType == vVNI || vowel != KEY_A)
        return false;
    int end = extraTyped ? _index - 1 : _index;
    if (end != 3)
        return false;
    if (CHR(1) != KEY_A || (TypingWord[1] & (TONE_MASK | TONEW_MASK | MARK_MASK)))
        return false;
    Uint16 c0 = CHR(0);
    Uint16 coda = CHR(2);
    bool isPlainD = (c0 == KEY_D && !(TypingWord[0] & TONE_MASK));
    if ((c0 == KEY_T && coda == KEY_P) || ((isPlainD || c0 == KEY_C) && coda == KEY_T)) {
        vowelAt = 1;
        return true;
    }
    return false;
}

void handleMainKey(const Uint16& data, const bool& isCaps) {
    //if is Z key, remove mark
    if (IS_KEY_Z(data)) {
        removeMark();
        if (!isChanged) {
            insertKey(data, isCaps);
        }
        return;
    }
    
    if (data == KEY_LEFT_BRACKET) { //standalone key [
        checkForStandaloneChar(data, isCaps, KEY_O);
        return;
    }
    
    if (data == KEY_RIGHT_BRACKET) { //standalone key }
        checkForStandaloneChar(data, isCaps, KEY_U);
        return;
    }
    
    // Check if initial consonants are valid
    int firstVowelIdx = 0;
    while (firstVowelIdx < _index && IS_CONSONANT(CHR(firstVowelIdx))) {
        firstVowelIdx++;
    }
    if (firstVowelIdx == 1 && CHR(0) == KEY_G && _index > 2 && CHR(1) == KEY_I && !IS_CONSONANT(CHR(2))) {
        firstVowelIdx = 2;
    }
    if (!isValidInitialConsonants(firstVowelIdx)) {
        if (data == KEY_W && vInputType != vSimpleTelex1) {
            checkForStandaloneChar(data, isCaps, KEY_U);
        } else {
            insertKey(data, isCaps);
        }
        return;
    }
    
    //if is D key
    if (IS_KEY_D(data)) {
        isCorect = false;
        isChanged = false;
        k = _index;
        for (i = 0; i < _consonantD.size(); i++) {
            if (_index < _consonantD[i].size())
                continue;
            isCorect = true;
            checkCorrectVowel(_consonantD, i, k, data);
            
            //allow d after consonant
            if (!isCorect && _index - 2 >= 0 && CHR(_index-1) == KEY_D && IS_CONSONANT(CHR(_index-2))) {
                isCorect = true;
            }
            if (isCorect) {
                isChanged = true;
                insertD(data, isCaps);
                break;
            }
        }
    
        if (!isChanged) {
            insertKey(data, isCaps);
        }
        return;
    }
    
    //if is mark key
    if (IS_MARK_KEY(data)) {
        int extraBackspace = 0;
        int vowelAt = -1;
        if (_index > 0 && deferredCircumflex(true, CHR(_index - 1), vowelAt)) {
            Uint16 coda = CHR(_index - 2);
            Uint16 last = CHR(_index - 1);
            bool hasMedial = (vowelAt >= 1) && !IS_CONSONANT(CHR(vowelAt - 1))
                && !(vowelAt - 1 == 1 && CHR(0) == KEY_Q && CHR(1) == KEY_U);
            // pieces, nieces: s after ie + c is the English plural.
            // tapas, papas: s after single a + p/t/c is the English plural.
            // f/r/x cannot sit on p/t/c.
            bool pieces = data == KEY_S && coda == KEY_C && last == KEY_E && (CHR(0) == KEY_P || CHR(0) == KEY_N);
            bool legalTone = coda == KEY_M
                ? (data == KEY_S || data == KEY_F || data == KEY_R || data == KEY_X || data == KEY_J)
                : (data == KEY_J || (data == KEY_S && hasMedial && !pieces));
            if (!legalTone) {
                insertKey(data, isCaps);
                return;
            }
            TypingWord[vowelAt] |= TONE_MASK;
            _index--;
            extraBackspace = 1;
        }
        for (auto& vowelEntry : _vowelForMark) {
            vector<vector<Uint16>>& charset = vowelEntry.second;
            isCorect = false;
            isChanged = false;
            k = _index;
            for (l = 0; l < charset.size(); l++) {
                if (_index < charset[l].size())
                    continue;
                isCorect = true;
                checkCorrectVowel(charset, l, k, data);

                if (isCorect) {
                    isChanged = true;
                    if (IS_KEY_S(data))
                        insertMark(MARK1_MASK);
                    else if (IS_KEY_F(data))
                        insertMark(MARK2_MASK);
                    else if (IS_KEY_R(data))
                        insertMark(MARK3_MASK);
                    else if (IS_KEY_X(data))
                        insertMark(MARK4_MASK);
                    else if (IS_KEY_J(data))
                        insertMark(MARK5_MASK);
                    break;
                }
            }

            if (isCorect) {
                break;
            }
        }

        if (!isChanged) {
            if (extraBackspace) {
                _index++;
                TypingWord[vowelAt] &= ~TONE_MASK;
            }
            insertKey(data, isCaps);
        } else if (extraBackspace) {
            deferredBackspace = extraBackspace;
        }

        return;
    }
    
    //check Vowel
    if (vInputType == vVNI) {
        VEI = 0;
        for (i = _index-1; i >= 0; i--) {
            if (CHR(i) == KEY_O || CHR(i) == KEY_A || CHR(i) == KEY_E) {
                VEI = i;
                break;
            }
        }
    }
    
    int deferredAt = -1;
    if (deferredCircumflex(false, data, deferredAt)) {
        // Keep the letter. The tone key, if it comes, adds the circumflex.
        insertKey(data, isCaps, false);
        tempDisableKey = false;
        return;
    }

    keyForAEO = ((vInputType != vVNI) ? data : ((data == KEY_7 || data == KEY_8 ? KEY_W : (data == KEY_6 ? (_index > 0 ? CHR(VEI) : 0) : data))));
    vector<vector<Uint16>>& charset = _vowel[keyForAEO];
    isCorect = false;
    isChanged = false;
    k = _index;
    for (i = 0; i < charset.size(); i++) {
        if (_index < charset[i].size())
            continue;
        isCorect = true;
        checkCorrectVowel(charset, i, k, data);
        
        if (isCorect) {
            if (IS_KEY_DOUBLE(data)) {
                isChanged = true;
                insertAOE(keyForAEO, isCaps);
            } else if (IS_KEY_W(data)) {
                if (vInputType == vVNI) {
                    for (j = _index-1; j >= 0; j--) {
                        if (CHR(j) == KEY_O || CHR(j) == KEY_U ||CHR(j) == KEY_A || CHR(j) == KEY_E) {
                            VEI = j;
                            break;
                        }
                    }
                    if ((data == KEY_7 && CHR(VEI) == KEY_A && (VEI-1>=0 ? CHR(VEI-1) != KEY_U : true)) || (data == KEY_8 && (CHR(VEI) == KEY_O || CHR(VEI) == KEY_U)))
                        break;
                }
                isChanged = true;
                insertW(keyForAEO, isCaps);
            }
            break;
        }
    }
    
    if (!isChanged) {
        if (data == KEY_W && vInputType != vSimpleTelex1) {
            checkForStandaloneChar(data, isCaps, KEY_U);
        } else {
            insertKey(data, isCaps);
        }
    }
}

void handleQuickTelex(const Uint16& data, const bool& isCaps) {
    hCode = vWillProcess;
    hBPC = 1;
    hNCC = 2;
    hData[1] = _quickTelex[data][0] | (isCaps ? CAPS_MASK : 0);
    hData[0] = _quickTelex[data][1] | (isCaps ? CAPS_MASK : 0);
    if (_index > 0)
        TypingWord[_index - 1] = hData[1];
    insertKey(_quickTelex[data][1], isCaps, false);
}

bool checkRestoreIfWrongSpelling(const int& handleCode) {
    for (ii = 0; ii < _index; ii++) {
        if ((!IS_CONSONANT(CHR(ii)) &&
             (TypingWord[ii] & MARK_MASK || TypingWord[ii] & TONE_MASK || TypingWord[ii] & TONEW_MASK)) ||
            (CHR(ii) == KEY_D && (TypingWord[ii] & TONE_MASK))) {
            int commonPrefix = 0;
            int maxPrefix = (_index < _stateIndex) ? _index : _stateIndex;
            while (commonPrefix < maxPrefix &&
                   TypingWord[commonPrefix] == KeyStates[commonPrefix] &&
                   !(TypingWord[commonPrefix] & (TONE_MASK | TONEW_MASK | MARK_MASK | STANDALONE_MASK))) {
                commonPrefix++;
            }
            hCode = handleCode;
            hBPC = _index - commonPrefix;
            hNCC = _stateIndex - commonPrefix;
            for (i = 0; i < _stateIndex; i++) {
                TypingWord[i] = KeyStates[i];
            }
            for (i = commonPrefix; i < _stateIndex; i++) {
                hData[_stateIndex - 1 - i] = TypingWord[i];
            }
            _index = _stateIndex;
            return true;
        }
    }
    return false;
}

void vTempOffSpellChecking() {
    if (_useSpellCheckingBefore) {
        vCheckSpelling = vCheckSpelling ? 0 : 1;
    }
}

void vSetCheckSpelling() {
    _useSpellCheckingBefore = vCheckSpelling;
}

void vTempOffEngine(const bool& off) {
    _willTempOffEngine = off;
}

bool checkQuickConsonant() {
    if (_index <= 1) return false;
    l = 0;
    if (_index > 0) {
        if (vQuickStartConsonant && _quickStartConsonant.find(CHR(0)) != _quickStartConsonant.end()) {
            hCode = vRestore;
            hBPC = _index;
            hNCC = _index + 1;
            if (_index < MAX_BUFF-1)
                _index++;
            //right shift
            for (i = _index-1; i >= 2; i--) {
                TypingWord[i] = TypingWord[i-1];
            }
            TypingWord[1] = _quickStartConsonant[CHR(0)][1] | ((TypingWord[0] & CAPS_MASK) && (TypingWord[2] & CAPS_MASK) ? CAPS_MASK : 0);
            TypingWord[0] = _quickStartConsonant[CHR(0)][0] | (TypingWord[0] & CAPS_MASK ? CAPS_MASK : 0);
            l = 1;;
        }
        if (vQuickEndConsonant &&
            (_index-2 >= 0 && !IS_CONSONANT(CHR(_index-2))) &&
            _quickEndConsonant.find(CHR(_index-1)) != _quickEndConsonant.end()) {
            hCode = vRestore;
            if (l == 1) {
                hNCC++;
            } else {
                hBPC = 1;
                hNCC = 2;
            }
            if (_index < MAX_BUFF-1)
                _index++;
            TypingWord[_index-1] = _quickEndConsonant[CHR(_index-2)][1] | (TypingWord[_index-2] & CAPS_MASK ? CAPS_MASK : 0);
            TypingWord[_index-2] = _quickEndConsonant[CHR(_index-2)][0] | (TypingWord[_index-2] & CAPS_MASK ? CAPS_MASK : 0);
            
            l = 1;
        }
        if (l == 1) {
            _hasHandleQuickConsonant = true;
            for (i = _index - 1; i >= 0; i--) {
                hData[_index - 1 - i] = GET(TypingWord[i]);
            }
            return true;
        }
    }
    return false;
}
/*==========================================================================================================*/

void vEnglishMode(const vKeyEventState& state, const Uint16& data, const bool& isCaps, const bool& otherControlKey) {
    hCode = vDoNothing;
    if (state == vKeyEventState::MouseDown || (otherControlKey && !isCaps)) {
        hMacroKey.clear();
        _willTempOffEngine = false;
    } else if (data == KEY_SPACE) {
        if (!_hasHandledMacro && findMacro(hMacroKey, hMacroData)) {
            hCode = vReplaceMaro;
            hBPC = (Byte)hMacroKey.size();
        }
        hMacroKey.clear();
        _willTempOffEngine = false;
    } else if (data == KEY_DELETE) {
        if (hMacroKey.size() > 0) {
            hMacroKey.pop_back();
        } else {
            _willTempOffEngine = false;
        }
    } else {
        if (isWordBreak(vKeyEvent::Keyboard, state, data) &&
            std::find(_charKeyCode.begin(), _charKeyCode.end(), data) == _charKeyCode.end()) {
            hMacroKey.clear();
            _willTempOffEngine = false;
        } else {
            if (!_willTempOffEngine)
                hMacroKey.push_back(data | (isCaps ? CAPS_MASK : 0));
        }
    }
}

void vKeyHandleEvent(const vKeyEvent& event,
                     const vKeyEventState& state,
                     const Uint16& data,
                     const Uint8& capsStatus,
                     const bool& otherControlKey) {
    _isCaps = (capsStatus == 1 || //shift
               capsStatus == 2); //caps lock
    if ((IS_NUMBER_KEY(data) && capsStatus == 1)
        || otherControlKey || isWordBreak(event, state, data) || (_index == 0 && IS_NUMBER_KEY(data))) {
        hCode = vDoNothing;
        hBPC = 0;
        hNCC = 0;
        hExt = 1; //word break
        
        //check macro feature
        if (vUseMacro && isMacroBreakCode(data) && !_hasHandledMacro && findMacro(hMacroKey, hMacroData)) {
            hCode = vReplaceMaro;
            hBPC = (Byte)hMacroKey.size();
            _hasHandledMacro = true;
        } else if ((vQuickStartConsonant || vQuickEndConsonant) && !tempDisableKey && isMacroBreakCode(data)) {
            checkQuickConsonant();
        } else if (vRestoreIfWrongSpelling && event != vKeyEvent::Mouse && isWordBreak(event, state, data)) { //restore key if wrong spelling with break-key
            if (!tempDisableKey && vCheckSpelling) {
                checkSpelling(true); //force check spelling
            }
            if (tempDisableKey && !(_spellingOK && _spellingVowelOK) && !checkRestoreIfWrongSpelling(vRestoreAndStartNewSession)) {
                hCode = vDoNothing;
            }
        }
        
        _isCharKeyCode = state == KeyDown && std::find(_charKeyCode.begin(), _charKeyCode.end(), data) != _charKeyCode.end();
        if (!_isCharKeyCode) { //clear all line cache
            _specialChar.clear();
            _typingStates.clear();
            _isInsideFileExtensionOrPath = false;
        } else { //check and save current word
            if (_spaceCount > 0) {
                saveWord(KEY_SPACE, _spaceCount);
                _spaceCount = 0;
            } else {
                saveWord();
            }
            _specialChar.push_back(data | (_isCaps ? CAPS_MASK : 0));
            hExt = 3;//normal word
        }
        
        if (hCode == vDoNothing) {
            startNewSession();
            vCheckSpelling = _useSpellCheckingBefore;
            _willTempOffEngine = false;
        } else if (hCode == vReplaceMaro || _hasHandleQuickConsonant) {
            _index = 0;
            _stateIndex = 0;
            tempDisableKey = false;
            _restoredOnPrevKey = false;
            _hasHandledMacro = false;
            _hasHandleQuickConsonant = false;
            _longWordHelper.clear();
        }
        
        //insert key for macro function
        if (vUseMacro) {
            if (_isCharKeyCode && hCode != vReplaceMaro) {
                hMacroKey.push_back(data | (_isCaps ? CAPS_MASK : 0));
            } else {
                hMacroKey.clear();
            }
        }
        
        if (vUpperCaseFirstChar) {
            if (data == KEY_DOT)
                _upperCaseStatus = 1;
            else if (data == KEY_ENTER || data == KEY_RETURN)
                _upperCaseStatus = 2;
            else
                _upperCaseStatus = 0;
        }
    } else if (data == KEY_SPACE) {
        if (!tempDisableKey && vCheckSpelling) {
            checkSpelling(true); //force check spelling
        }
        if (vUseMacro && !_hasHandledMacro && findMacro(hMacroKey, hMacroData)) { //macro
            hCode = vReplaceMaro;
            hBPC = (Byte)hMacroKey.size();
            _spaceCount++;
            _hasHandledMacro = true;
        } else if ((vQuickStartConsonant || vQuickEndConsonant) && !tempDisableKey && checkQuickConsonant()) {
            _spaceCount++;
        } else if (vRestoreIfWrongSpelling && tempDisableKey && !(_spellingOK && _spellingVowelOK) && !_hasHandledMacro) { //restore key if wrong spelling
            if (!checkRestoreIfWrongSpelling(vRestore)) {
                hCode = vDoNothing;
            }
            _spaceCount++;
        } else { //do nothing with SPACE KEY
            hCode = vDoNothing;
            _spaceCount++;
        }
        if (vUseMacro) {
            hMacroKey.clear();
        }
        if (vUpperCaseFirstChar && _upperCaseStatus == 1) {
            _upperCaseStatus = 2;
        }
        //save word
        if (_spaceCount == 1) {
            if (_specialChar.size() > 0) {
                saveSpecialChar();
            } else {
                saveWord();
            }
        }
        vCheckSpelling = _useSpellCheckingBefore;
        _willTempOffEngine = false;
        // An English word such as "motion" turns composition off for that word only.
        tempDisableKey = false;
        _restoredOnPrevKey = false;
    } else if (data == KEY_DELETE) {
        _restoredOnPrevKey = false;
        hCode = vDoNothing;
        hExt = 2; //delete
        if (_specialChar.size() > 0) {
            _specialChar.pop_back();
            if (_specialChar.size() == 0) {
                restoreLastTypingState();
            }
        } else if (_spaceCount > 0) { //previous char is space
            _spaceCount--;
            if (_spaceCount == 0) { //restore word
                restoreLastTypingState();
            }
        } else {
            if (_stateIndex > 0) {
                _stateIndex--;
            }
            if (_index > 0){
                _index--;
                if (_longWordHelper.size() > 0) {
                    //right shift
                    for (i = MAX_BUFF - 1; i > 0; i--) {
                        TypingWord[i] = TypingWord[i-1];
                    }
                    TypingWord[0] = _longWordHelper.back();
                    _longWordHelper.pop_back();
                    _index++;
                }
                syncKeyStatesFromTypingWord();
                if (vCheckSpelling)
                    checkSpelling();
                if (_index == 0) {
                    startNewSession();
                    _specialChar.clear();
                } else { //August 23rd continue check grammar
                    checkGrammar(1);
                }
            } else {
                // _index was already 0 when Backspace was pressed:
                // user is backspacing across the word boundary!
                startNewSession();
                _specialChar.clear();
                restoreLastTypingState();
                if (_spaceCount > 0) {
                    _spaceCount--;
                    if (_spaceCount == 0) {
                        restoreLastTypingState();
                    }
                } else if (_index > 0) {
                    _index--;
                    syncKeyStatesFromTypingWord();
                    if (vCheckSpelling)
                        checkSpelling();
                }
            }
            if (vUseMacro && hMacroKey.size() > 0) {
                hMacroKey.pop_back();
            }
            
            hBPC = 0;
            hNCC = 0;
            hExt = 2; //delete key
        }
    } else { //START AND CHECK KEY
        if (_spaceCount > 0) {
            hBPC = 0;
            hNCC = 0;
            hExt = 0;
            startNewSession();
            //continute save space
            saveWord(KEY_SPACE, _spaceCount);
            _spaceCount = 0;
            _isInsideFileExtensionOrPath = false;
        }
        if (_willTempOffEngine) {
            hCode = vDoNothing;
            hExt = 3;
            return;
        }
        if (_specialChar.size() > 0) {
            Uint32 lastSpecialEntry = _specialChar.back();
            Uint16 lastSpecial = lastSpecialEntry & CHAR_MASK;
            bool isShift = (lastSpecialEntry & CAPS_MASK) != 0;
            if (_specialChar.size() == 1 && !isShift && (lastSpecial == KEY_DOT || lastSpecial == KEY_SLASH || lastSpecial == KEY_BACK_SLASH)) {
                _isInsideFileExtensionOrPath = true;
            }
            saveSpecialChar();
        }

        deferredBackspace = 0;
        insertState(data, _isCaps); //save state
        
        // c+h, n+h, n+g are one ending. The tone key may sit between the two letters
        // (thicsh -> thích, bansh -> bánh, tiensg -> tiếng, tienegs -> tiếng). That is still Vietnamese.
        bool completingCoda = false;
        if (_index >= 2 && !IS_CONSONANT(CHR(_index - 2))) {
            Uint16 prevV = CHR(_index - 2);
            Uint32 prevVTone = TypingWord[_index - 2] & (TONE_MASK | TONEW_MASK);
            Uint32 prevVMark = TypingWord[_index - 2] & MARK_MASK;
            bool atMostOneInterveningKey = _stateIndex < 3 ||
                (KeyStates[_stateIndex - 2] & CHAR_MASK) == CHR(_index - 1) ||
                (KeyStates[_stateIndex - 3] & CHAR_MASK) == CHR(_index - 1);
            if (atMostOneInterveningKey) {
                if ((CHR(_index - 1) == KEY_C || CHR(_index - 1) == KEY_N) && data == KEY_H) {
                    bool validChNhVowel =
                        (prevV == KEY_A && !(prevVTone & TONE_MASK) && !(_index >= 3 && !IS_CONSONANT(CHR(_index - 3)) && prevVMark != 0)) ||
                        (prevV == KEY_E && (prevVTone == TONE_MASK || prevVMark == 0)) ||
                        prevV == KEY_I || prevV == KEY_Y;
                    completingCoda = validChNhVowel;
                } else if (CHR(_index - 1) == KEY_N && data == KEY_G) {
                    completingCoda = (prevV != KEY_I && prevV != KEY_Y);
                }
            }
        }
        // thuajat -> thuật, viejet -> việt, chuyenje -> chuyện, bojo -> bộ, bofo -> bồ, tajap -> tập, nhajat -> nhật.
        // A second a/e/o after a mark key adds â/ê/ô in a diphthong (iê, yê, êu, uô, ôi, uâ, oâ, âu, ây),
        // on open o/e with huyền/nặng (bojo -> bộ, bofo -> bồ, thefe -> thề, leje -> lệ, ddefe -> đề, excluding mojo/refer/defer/reject),
        // or on a with nặng before a coda (tajap -> tập, nhajat -> nhật, excluding ajar/raja/pajama).
        bool repeatsMarkedVowel = false;
        if (data == KEY_A || data == KEY_E || data == KEY_O) {
            bool same = false;
            bool pair = false;
            bool hasFOrJ = false;
            bool hasJ = false;
            int firstVowelIdx = -1;
            for (int si = _index - 1; si >= 0; si--) {
                if (IS_CONSONANT(CHR(si)))
                    break;
                firstVowelIdx = si;
                if (CHR(si) == data && !(TypingWord[si] & (TONE_MASK | TONEW_MASK))) {
                    same = true;
                    if (TypingWord[si] & (MARK2_MASK | MARK5_MASK))
                        hasFOrJ = true;
                    if (TypingWord[si] & MARK5_MASK)
                        hasJ = true;
                }
                if (data == KEY_E && (CHR(si) == KEY_I || CHR(si) == KEY_Y || CHR(si) == KEY_U))
                    pair = true;
                if (data == KEY_O && ((CHR(si) == KEY_U && !(si == 1 && CHR(0) == KEY_Q)) || CHR(si) == KEY_I))
                    pair = true;
                if (data == KEY_A && (CHR(si) == KEY_U || CHR(si) == KEY_O || CHR(si) == KEY_Y) && CHR(_index - 1) != KEY_I)
                    pair = true;
            }
            bool isPlainD0 = CHR(0) == KEY_D && !(TypingWord[0] & TONE_MASK);
            bool singleMarkStep = (_stateIndex < 3 || !IS_MARK_KEY((KeyStates[_stateIndex - 3] & CHAR_MASK)));
            bool allowSingleO = singleMarkStep && data == KEY_O && hasFOrJ && firstVowelIdx > 0 && CHR(0) != KEY_M && !isPlainD0;
            bool allowSingleE = singleMarkStep && data == KEY_E && hasFOrJ && firstVowelIdx > 0 &&
                CHR(0) != KEY_R && !isPlainD0 && CHR(0) != KEY_P;
            bool allowSingleA = singleMarkStep && data == KEY_A && hasJ && firstVowelIdx > 0 &&
                CHR(0) != KEY_R && CHR(0) != KEY_P;
            repeatsMarkedVowel = same && (pair || allowSingleO || allowSingleE || allowSingleA);
        }
        // tienje -> tiện, thuatja -> thuật, thata -> thât, nene -> nên, mojto -> một, tosto -> tốt, tapja -> tập.
        // The second vowel sits after the coda in a diphthong or valid single-vowel delayed circumflex.
        bool repeatVowelBeforeCoda = false;
        if ((data == KEY_A || data == KEY_E || data == KEY_O) && _index >= 1 && IS_CONSONANT(CHR(_index - 1))) {
            int codaStart = _index - 1;
            while (codaStart >= 0 && IS_CONSONANT(CHR(codaStart)))
                codaStart--;
            if (codaStart >= 0 && canDelayedCircumflex(codaStart, data)) {
                repeatVowelBeforeCoda = true;
            }
        }
        // Complete valid Vietnamese diphthongs ending in -a or -o after a tone mark:
        // -a: ua/ưa/qua (của, múa, mùa, lụa, quả, sửa), ia (phía, chìa, tỉa, nghĩa, địa), oa (hoá, hoà, hoả, hoạ, toán), uya (khuya)
        // -o: ươ, ao (báo, vào, bảo, bão, tạo), eo (kéo, mèo, kẻo, kẽo, kẹo), gio (gió, giò, giỏ, giõ, giọ)
        bool completesUa = false;
        if ((data == KEY_A || data == KEY_O) && _index >= 1) {
            for (int si = 0; si < _index; si++) {
                if (CHR(si) == KEY_U && (TypingWord[si] & TONEW_MASK) && !(si == 0 && (TypingWord[si] & STANDALONE_MASK))) {
                    completesUa = true;
                    break;
                }
            }
            if (!completesUa && _index >= 2) {
                Uint16 lastV = CHR(_index - 1);
                bool isPlainD0 = CHR(0) == KEY_D && !(TypingWord[0] & TONE_MASK);
                if (data == KEY_A) {
                    if (lastV == KEY_U && !(_index == 1)) {
                        completesUa = true;
                    } else if (lastV == KEY_I && !isPlainD0 && !(CHR(0) == KEY_V && (TypingWord[_index - 1] & MARK1_MASK))) {
                        completesUa = true;
                    } else if (lastV == KEY_O && !(TypingWord[_index - 1] & (TONE_MASK | TONEW_MASK)) &&
                               !(_index == 2 && CHR(0) == KEY_S && (TypingWord[1] & MARK2_MASK))) {
                        completesUa = true;
                    } else if (lastV == KEY_Y && CHR(_index - 2) == KEY_U) {
                        completesUa = true;
                    }
                } else if (data == KEY_O) {
                    if (lastV == KEY_A && !(TypingWord[_index - 1] & (TONE_MASK | TONEW_MASK)) &&
                        !(_index == 2 && CHR(0) == KEY_M && (TypingWord[1] & MARK5_MASK))) {
                        completesUa = true;
                    } else if (lastV == KEY_E && !(TypingWord[_index - 1] & TONE_MASK) &&
                               !(_index == 2 && ((CHR(0) == KEY_H && (TypingWord[1] & MARK3_MASK)) || CHR(0) == KEY_R || isPlainD0))) {
                        completesUa = true;
                    } else if (_index == 2 && CHR(0) == KEY_G && lastV == KEY_I) {
                        completesUa = true;
                    }
                }
            }
        }
        
        // Check if previous key was a Telex mark key (S, R, X, F) that added a tone mark,
        // and current key produces an impossible Vietnamese cluster/sequence but a common English word.
        // Restore the entire word to its raw keystrokes!
        // w after a tone still modifies the vowel (hopjw -> hợp, muotjw -> mượt).
        // It is not an English continuation of the tone key.
        // Pressing the same tone key again removes the mark (dunss -> duns). Do not
        // dump both keystrokes, or the letter appears twice.
        bool repeatTone = _stateIndex >= 2
            && (KeyStates[_stateIndex - 2] & CHAR_MASK) == data
            && (data == KEY_S || data == KEY_F || data == KEY_R || data == KEY_X || data == KEY_J);
        bool hasPlainD = data == KEY_D && _index >= 1 && CHR(0) == KEY_D && !(TypingWord[0] & TONE_MASK);
        if (vInputType != vVNI && tempDisableKey && _restoredOnPrevKey) {
            _restoredOnPrevKey = false;
            Uint16 prevTone = (_stateIndex >= 2) ? (KeyStates[_stateIndex - 2] & CHAR_MASK) : 0;
            bool wasAdjacentRepeat = _stateIndex >= 3 && (KeyStates[_stateIndex - 3] & CHAR_MASK) == prevTone;
            bool dropDuplicateTone = wasAdjacentRepeat &&
                (prevTone == KEY_R && IS_CONSONANT(data) && data != KEY_H);
            if (!dropDuplicateTone) {
                int commonPrefix = 0;
                int maxPrefix = (_index < _stateIndex) ? _index : _stateIndex;
                while (commonPrefix < maxPrefix &&
                       TypingWord[commonPrefix] == KeyStates[commonPrefix] &&
                       !(TypingWord[commonPrefix] & (TONE_MASK | TONEW_MASK | MARK_MASK | STANDALONE_MASK))) {
                    commonPrefix++;
                }
                hCode = vWillProcess;
                hBPC = _index - commonPrefix;
                hNCC = _stateIndex - commonPrefix;
                for (i = 0; i < _stateIndex; i++) {
                    TypingWord[i] = KeyStates[i];
                }
                for (i = commonPrefix; i < _stateIndex; i++) {
                    hData[_stateIndex - 1 - i] = TypingWord[i];
                }
                _index = _stateIndex;
                _spellingOK = true;
                if (vUseMacro) {
                    for (i = 0; i < hBPC; i++) {
                        if (hMacroKey.size() > 0)
                            hMacroKey.pop_back();
                    }
                    for (i = commonPrefix; i < _stateIndex; i++) {
                        hMacroKey.push_back(TypingWord[i]);
                    }
                }
                if (vUpperCaseFirstChar) {
                    if (_index == 1 && _upperCaseStatus == 2) {
                        upperCaseFirstCharacter();
                    }
                    _upperCaseStatus = 0;
                }
                return;
            } else {
                KeyStates[_stateIndex - 2] = KeyStates[_stateIndex - 1];
                _stateIndex--;
            }
        }
        bool canWMod = (data == KEY_W && vInputType != vVNI) ? canKeyWModify() : false;
        bool hasDelayedCircumflex = false;
        if (repeatTone) {
            for (int si = 0; si < _index; si++) {
                if (TypingWord[si] & TONE_MASK) {
                    Uint16 vc = CHR(si);
                    bool adj = false;
                    for (int ki = 1; ki < (int)_stateIndex - 2; ki++) {
                        if ((KeyStates[ki] & CHAR_MASK) == vc && (KeyStates[ki - 1] & CHAR_MASK) == vc) {
                            adj = true;
                            break;
                        }
                    }
                    if (!adj) {
                        hasDelayedCircumflex = true;
                        break;
                    }
                }
            }
        }
        if (vInputType != vVNI && !tempDisableKey && !_isInsideFileExtensionOrPath && _stateIndex >= 2 && !canWMod && (!repeatTone || hasDelayedCircumflex) && !hasPlainD) {
            Uint16 prevKey = KeyStates[_stateIndex - 2] & CHAR_MASK;
            bool shouldRestore = hasDelayedCircumflex;
            bool hasAnyMark = false;
            Uint32 allVowelMarks = 0;
            bool hasAnyToneMod = false;
            for (int si = 0; si < _index; si++) {
                if (TypingWord[si] & MARK_MASK) {
                    hasAnyMark = true;
                    allVowelMarks |= (TypingWord[si] & MARK_MASK);
                }
                if (TypingWord[si] & (TONE_MASK | TONEW_MASK)) {
                    hasAnyToneMod = true;
                }
            }
            if (data == KEY_Z) {
                if (_index >= 1 && CHR(0) == KEY_U && (TypingWord[0] & STANDALONE_MASK)) {
                    shouldRestore = true;
                }
            }
            Uint32 incomingMark = (data == KEY_S) ? MARK1_MASK :
                                  (data == KEY_F) ? MARK2_MASK :
                                  (data == KEY_R) ? MARK3_MASK :
                                  (data == KEY_X) ? MARK4_MASK :
                                  (data == KEY_J) ? MARK5_MASK : 0;
            // If previous key was Z that stripped a mark, a following non-mark letter indicates English (borzoi, terzo, mirza, befezzed).
            if (!shouldRestore && prevKey == KEY_Z && _stateIndex >= _index + 2 && incomingMark == 0) {
                shouldRestore = true;
            }
            // Open initial P with modified vowel (pô, pâ): mark keys are blocked by checkCorrectVowel, so r/s/f/x/j/h indicate English (poor, poorly, poof, pooh).
            if (!shouldRestore && _index >= 2 && CHR(0) == KEY_P && !IS_CONSONANT(CHR(1)) && !IS_CONSONANT(CHR(_index - 1)) &&
                (incomingMark != 0 || data == KEY_H)) {
                shouldRestore = true;
            }
            // Stop codas (p, t, c) can never combine with huyền (f), hỏi (r), or ngã (x) (cofactor, coracle, borate, boracic, curate, duration).
            if (!shouldRestore && !hasAnyToneMod && _index >= 1 && !IS_CONSONANT(CHR(_index - 1)) &&
                (data == KEY_P || data == KEY_T || data == KEY_C) &&
                (allVowelMarks & (MARK2_MASK | MARK3_MASK | MARK4_MASK))) {
                shouldRestore = true;
            }
            if (incomingMark != 0) {
                bool hasSameMark = false;
                for (int si = 0; si < _index; si++) {
                    if ((TypingWord[si] & MARK_MASK) == incomingMark) {
                        hasSameMark = true;
                        break;
                    }
                }
                if (hasSameMark) {
                    // Only restore all keys from KeyStates if a vowel was typed after the earlier mark key
                    // (e.g., ba-s-i-s -> basis, oa-s-i-s -> oasis, di-s-a-s-t-e-r -> disaster).
                    // If only consonants were typed after the earlier mark key (e.g., bu-s-n-s -> buns),
                    // let insertMark cancel the mark and append one literal character.
                    bool vowelAfterEarlierMark = false;
                    for (int ki = (int)_stateIndex - 2; ki >= 0; ki--) {
                        if ((KeyStates[ki] & CHAR_MASK) == data) {
                            for (int vj = ki + 1; vj < (int)_stateIndex - 1; vj++) {
                                Uint16 vk = KeyStates[vj] & CHAR_MASK;
                                if (!IS_CONSONANT(vk)) {
                                    vowelAfterEarlierMark = true;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if (vowelAfterEarlierMark) {
                        shouldRestore = true;
                    }
                }
            }
            // Universal phonological check for horned/breve vowels (ă, ơ, ư) at the end of TypingWord:
            // - ă: only codas c, m, n, p, t can follow (away, await, award, awful restore).
            // - ơ: only c, m, n, p, t, i (plus u when preceded by u/ư for ươu) can follow (norway, power, tower, lower, toward restore).
            // - ư (after an initial consonant): only c, m, n, t, a, o, u, i (excluding i after standalone sw- for swim/switch) can follow (sweet, swear, sweep restore).
            if (!shouldRestore && _index >= 1 && !IS_CONSONANT(CHR(_index - 1)) && (TypingWord[_index - 1] & TONEW_MASK)) {
                bool isToneOrMod = data == KEY_S || data == KEY_F || data == KEY_R || data == KEY_X
                    || data == KEY_J || (data == KEY_Z && hasAnyMark) || canWMod
                    || (data == KEY_D && _index >= 1 && CHR(0) == KEY_D);
                if (!isToneOrMod) {
                    Uint16 vLast = CHR(_index - 1);
                    if (vLast == KEY_A) {
                        if (data != KEY_C && data != KEY_M && data != KEY_N && data != KEY_P && data != KEY_T)
                            shouldRestore = true;
                    } else if (vLast == KEY_O) {
                        bool hasPrecedingU = _index >= 2 && CHR(_index - 2) == KEY_U && !(_index == 2 && CHR(0) == KEY_Q);
                        bool okO = data == KEY_C || data == KEY_M || data == KEY_N || data == KEY_P || data == KEY_T
                            || data == KEY_I || (hasPrecedingU && data == KEY_U);
                        if (!okO)
                            shouldRestore = true;
                    } else if (vLast == KEY_U && !(_index == 1 && (TypingWord[0] & STANDALONE_MASK))) {
                        bool isSwi = _index == 2 && CHR(0) == KEY_S && (TypingWord[1] & STANDALONE_MASK) && data == KEY_I;
                        bool okU = (data == KEY_C || data == KEY_M || data == KEY_N || data == KEY_T
                            || data == KEY_A || data == KEY_O || data == KEY_U || data == KEY_I) && !isSwi;
                        if (!okU)
                            shouldRestore = true;
                    }
                }
            }
            bool hasCodaAtEnd = _index >= 2 && IS_CONSONANT(CHR(_index - 1)) && !completingCoda;
            bool isStopCodaAtEnd = _index >= 2 && (CHR(_index - 1) == KEY_P || CHR(_index - 1) == KEY_T || CHR(_index - 1) == KEY_C
                || (CHR(_index - 1) == KEY_H && CHR(_index - 2) == KEY_C));
            // If prevKey just formed a single-vowel delayed circumflex on a syllable that already had a mark before prevKey,
            // a trailing mark key indicates an English word (censer, densher, temser, tonsor, kermes, hormos, khanjar).
            if (!shouldRestore && incomingMark != 0 && hasCodaAtEnd &&
                (prevKey == KEY_A || prevKey == KEY_E || prevKey == KEY_O)) {
                for (int si = 0; si < _index - 1; si++) {
                    if (CHR(si) == prevKey && (TypingWord[si] & TONE_MASK) && (TypingWord[si] & MARK_MASK)) {
                        bool singleV = (si == 0 || IS_CONSONANT(CHR(si - 1)) || (si == 2 && CHR(0) == KEY_Q && CHR(1) == KEY_U));
                        if (singleV) {
                            shouldRestore = true;
                            break;
                        }
                    }
                }
            }
            bool hasInitialRawW = _stateIndex >= 1 && (KeyStates[0] & CHAR_MASK) == KEY_W;
            bool hasVietMod = (_index >= 1 && CHR(0) == KEY_D && (TypingWord[0] & TONE_MASK));
            if (!hasVietMod && !hasInitialRawW) {
                int vCount = 0;
                bool hasCircumflex = false;
                for (int si = 0; si < _index; si++) {
                    if (!IS_CONSONANT(CHR(si)) && !(si == 1 && (CHR(0) == KEY_Q || CHR(0) == KEY_G))) {
                        vCount++;
                    }
                    if (TypingWord[si] & TONEW_MASK) {
                        hasVietMod = true;
                        break;
                    }
                    if (TypingWord[si] & TONE_MASK) {
                        if (CHR(si) == KEY_A) {
                            hasVietMod = true;
                            break;
                        }
                        hasCircumflex = true;
                    }
                }
                if (hasCircumflex && vCount >= 2) {
                    hasVietMod = true;
                }
            }
            Uint16 lastVowel = (_index >= 1) ? CHR(_index - 1) : 0;
            bool isEnglishRf = prevKey == KEY_R && data == KEY_F && _index == 2 &&
                ((lastVowel == KEY_U && (CHR(0) == KEY_S || CHR(0) == KEY_T)) ||
                 (lastVowel == KEY_E && !(TypingWord[1] & TONE_MASK) && (CHR(0) == KEY_S || CHR(0) == KEY_K)));
            bool isEnglishFs = prevKey == KEY_F && data == KEY_S && (
                (lastVowel == KEY_E && ((_index == 3 && CHR(0) == KEY_C && CHR(1) == KEY_H && !(TypingWord[2] & TONE_MASK)) ||
                                        (_index == 2 && CHR(0) == KEY_R))) ||
                (lastVowel == KEY_O && _index == 2 && (TypingWord[1] & TONE_MASK) &&
                 (CHR(0) == KEY_R || CHR(0) == KEY_H || CHR(0) == KEY_G))
            );
            bool isEnglishOpenCluster = !hasCodaAtEnd && !hasVietMod && (
                (prevKey == KEY_R && data == KEY_S) ||
                isEnglishRf ||
                isEnglishFs
            );
            bool isValidToneChange = incomingMark != 0 && (!isStopCodaAtEnd || data == KEY_S || data == KEY_J) && !isEnglishOpenCluster;
            if (!shouldRestore && !(data == KEY_Z && hasAnyMark) && !isValidToneChange && prevKey == KEY_S && !repeatVowelBeforeCoda) {
                // If word already had an ending consonant before S was pressed (e.g. "c-a-n-s" -> cán):
                // the Vietnamese syllable is already complete! No letter can ever follow.
                // Any following letter indicates an English word (e.g. "artist", "first", "trans").
                if (_index >= 2 && IS_CONSONANT(CHR(_index - 1)) && !completingCoda) {
                    for (int si = 0; si < _index - 1; si++) {
                        if (TypingWord[si] & MARK1_MASK) {
                            shouldRestore = true;
                            break;
                        }
                    }
                } else if (_index >= 1 && !completingCoda) {
                    bool allowEAfterUy = _index >= 2 && CHR(_index - 1) == KEY_Y && CHR(_index - 2) == KEY_U;
                    for (int si = 0; si < _index; si++) {
                        if (TypingWord[si] & MARK1_MASK) {
                            Uint16 vChar = CHR(si);
                            bool hasPrecedingUw = si >= 1 && CHR(si - 1) == KEY_U && (TypingWord[si - 1] & TONEW_MASK);
                            bool isBusy = _index == 2 && CHR(0) == KEY_B && vChar == KEY_U && data == KEY_Y;
                            if (data == KEY_K || data == KEY_H || data == KEY_L || data == KEY_D ||
                                data == KEY_R || data == KEY_F || data == KEY_W || data == KEY_Z ||
                                data == KEY_J || data == KEY_B || data == KEY_V || data == KEY_Q ||
                                (data == KEY_T && _index == 3 && CHR(0) == KEY_G && CHR(1) == KEY_U && CHR(2) == KEY_E && !(TypingWord[2] & TONE_MASK)) ||
                                (data == KEY_E && !repeatsMarkedVowel && !allowEAfterUy) ||
                                (data == KEY_A && !repeatsMarkedVowel && !completesUa) ||
                                (data == KEY_O && !repeatsMarkedVowel && !completesUa) ||
                                (vChar == KEY_E && !(TypingWord[si] & TONE_MASK) && (data == KEY_I || data == KEY_U || data == KEY_A || data == KEY_Y)) ||
                                (vChar == KEY_I && (data == KEY_I || (data == KEY_O && !completesUa) || data == KEY_Y)) ||
                                (vChar == KEY_O && !(TypingWord[si] & (TONE_MASK | TONEW_MASK)) && ((data == KEY_U && !hasPrecedingUw) || (data == KEY_Y && si == _index - 1))) ||
                                (vChar == KEY_U && si == _index - 1 && !(TypingWord[si] & TONEW_MASK) && data == KEY_U) ||
                                isBusy) {
                                shouldRestore = true;
                            }
                            break;
                        }
                    }
                }
            } else if (!shouldRestore && !(data == KEY_Z && hasAnyMark) && !isValidToneChange && prevKey == KEY_R && !repeatVowelBeforeCoda) {
                if (_index >= 2 && IS_CONSONANT(CHR(_index - 1)) && !completingCoda) {
                    for (int si = 0; si < _index - 1; si++) {
                        if (TypingWord[si] & MARK3_MASK) {
                            shouldRestore = true;
                            break;
                        }
                    }
                } else if (_index >= 1 && !completingCoda) {
                    for (int si = 0; si < _index; si++) {
                        if (TypingWord[si] & MARK3_MASK) {
                            Uint16 vChar = CHR(si);
                            bool hasPrecedingUw = si >= 1 && CHR(si - 1) == KEY_U && (TypingWord[si - 1] & TONEW_MASK);
                            bool invalidGlideAfterR = false;
                            if (data == KEY_I || data == KEY_U || data == KEY_Y) {
                                if (vChar == KEY_E && (data == KEY_I || data == KEY_Y || !(TypingWord[si] & TONE_MASK))) {
                                    invalidGlideAfterR = true;
                                } else if (vChar == KEY_O && ((data == KEY_U && !hasPrecedingUw) || (data == KEY_Y && si == _index - 1))) {
                                    invalidGlideAfterR = true;
                                } else if (vChar == KEY_I && (data == KEY_I || data == KEY_Y)) {
                                    invalidGlideAfterR = true;
                                } else if (vChar == KEY_U && ((data == KEY_U && si == _index - 1 && !(TypingWord[si] & TONEW_MASK)) || (data == KEY_Y && _index == 2 && CHR(0) == KEY_B))) {
                                    invalidGlideAfterR = true;
                                } else if (_index >= 2 && !IS_CONSONANT(CHR(_index - 1)) && !IS_CONSONANT(CHR(_index - 2))) {
                                    Uint16 pv0 = CHR(_index - 2);
                                    Uint16 pv1 = CHR(_index - 1);
                                    bool isQuOrGi = (_index >= 3 && CHR(_index - 3) == KEY_Q && pv0 == KEY_U)
                                        || (_index >= 3 && CHR(_index - 3) == KEY_G && pv0 == KEY_I);
                                    if (!isQuOrGi) {
                                        bool validTriphthong = (pv0 == KEY_U && pv1 == KEY_Y && data == KEY_U)
                                            || (pv0 == KEY_U && pv1 == KEY_O && (data == KEY_I || data == KEY_U))
                                            || (pv0 == KEY_O && pv1 == KEY_A && (data == KEY_I || data == KEY_Y));
                                        if (!validTriphthong) {
                                            invalidGlideAfterR = true;
                                        }
                                    }
                                }
                            }
                            if ((data != KEY_M && data != KEY_N && data != KEY_I && data != KEY_U && data != KEY_Y && data != KEY_R && !completesUa && !repeatsMarkedVowel) || invalidGlideAfterR) {
                                shouldRestore = true;
                            }
                            break;
                        }
                    }
                }
            } else if (!shouldRestore && !(data == KEY_Z && hasAnyMark) && !isValidToneChange && prevKey == KEY_J && !repeatVowelBeforeCoda) {
                if (_index >= 2 && IS_CONSONANT(CHR(_index - 1)) && !completingCoda) {
                    for (int si = 0; si < _index - 1; si++) {
                        if (TypingWord[si] & MARK5_MASK) {
                            shouldRestore = true;
                            break;
                        }
                    }
                } else if (_index >= 1 && !completingCoda) {
                    for (int si = 0; si < _index; si++) {
                        if (TypingWord[si] & MARK5_MASK) {
                            Uint16 vChar = CHR(si);
                            bool hasPrecedingUw = si >= 1 && CHR(si - 1) == KEY_U && (TypingWord[si - 1] & TONEW_MASK);
                            bool invalidGlideAfterJ = (data == KEY_I || data == KEY_U || data == KEY_Y) && (
                                (vChar == KEY_E && (data == KEY_I || data == KEY_Y || !(TypingWord[si] & TONE_MASK))) ||
                                (vChar == KEY_O && ((data == KEY_U && !hasPrecedingUw) || (data == KEY_Y && si == _index - 1))) ||
                                (vChar == KEY_I && (data == KEY_I || data == KEY_Y)) ||
                                (vChar == KEY_U && si == _index - 1 && data == KEY_U && !(TypingWord[si] & TONEW_MASK))
                            );
                            if ((data != KEY_P && data != KEY_C && data != KEY_M && data != KEY_T && data != KEY_N &&
                                 data != KEY_I && data != KEY_U && data != KEY_Y && data != KEY_J && !completesUa && !repeatsMarkedVowel) ||
                                invalidGlideAfterJ) {
                                shouldRestore = true;
                            }
                            break;
                        }
                    }
                }
            } else if (!shouldRestore && !(data == KEY_Z && hasAnyMark) && !isValidToneChange && prevKey == KEY_X && !repeatVowelBeforeCoda) {
                if (_index >= 2 && IS_CONSONANT(CHR(_index - 1)) && !completingCoda) {
                    for (int si = 0; si < _index - 1; si++) {
                        if (TypingWord[si] & MARK4_MASK) {
                            shouldRestore = true;
                            break;
                        }
                    }
                } else if (_index >= 1 && !completingCoda) {
                    for (int si = 0; si < _index; si++) {
                        if (TypingWord[si] & MARK4_MASK) {
                            Uint16 vChar = CHR(si);
                            bool hasPrecedingUw = si >= 1 && CHR(si - 1) == KEY_U && (TypingWord[si - 1] & TONEW_MASK);
                            bool isTaxi = _index == 2 && CHR(0) == KEY_T && vChar == KEY_A && !(TypingWord[1] & (TONE_MASK | TONEW_MASK)) && data == KEY_I;
                            bool invalidGlideAfterX = false;
                            if (data == KEY_I || data == KEY_U || data == KEY_Y) {
                                if (vChar == KEY_E && (data == KEY_I || data == KEY_Y || !(TypingWord[si] & TONE_MASK))) {
                                    invalidGlideAfterX = true;
                                } else if (vChar == KEY_O && ((data == KEY_U && !hasPrecedingUw) || (data == KEY_Y && si == _index - 1))) {
                                    invalidGlideAfterX = true;
                                } else if (vChar == KEY_I && (data == KEY_I || data == KEY_Y)) {
                                    invalidGlideAfterX = true;
                                } else if (vChar == KEY_U && si == _index - 1 && data == KEY_U && !(TypingWord[si] & TONEW_MASK)) {
                                    invalidGlideAfterX = true;
                                } else if (isTaxi) {
                                    invalidGlideAfterX = true;
                                }
                            }
                            if ((data != KEY_N && data != KEY_M && data != KEY_I && data != KEY_U && data != KEY_Y && data != KEY_X && !completesUa && !repeatsMarkedVowel) || invalidGlideAfterX) {
                                shouldRestore = true;
                            }
                            break;
                        }
                    }
                }
            } else if (!shouldRestore && !(data == KEY_Z && hasAnyMark) && !isValidToneChange && prevKey == KEY_F && !repeatVowelBeforeCoda) {
                if (_index >= 2 && IS_CONSONANT(CHR(_index - 1)) && !completingCoda) {
                    for (int si = 0; si < _index - 1; si++) {
                        if (TypingWord[si] & MARK2_MASK) {
                            shouldRestore = true;
                            break;
                        }
                    }
                } else if (_index >= 1 && !completingCoda) {
                    for (int si = 0; si < _index; si++) {
                        if (TypingWord[si] & MARK2_MASK) {
                            Uint16 vChar = CHR(si);
                            bool hasPrecedingUw = si >= 1 && CHR(si - 1) == KEY_U && (TypingWord[si - 1] & TONEW_MASK);
                            if (vChar == KEY_E) {
                                // "rè" (ref) followed by vowels or non-m/n consonants indicates English (refer, refactor, refresh, reflex, reflect, reform, refuse, refine),
                                // while ê + u (chiều, nhiều, đều, lều) and e + o (mèo, bèo, kèo, nghèo, đèo) are Vietnamese.
                                bool allowEu = (TypingWord[si] & TONE_MASK) && data == KEY_U;
                                if (data != KEY_M && data != KEY_N && data != KEY_F && !repeatsMarkedVowel && !completesUa && !allowEu) {
                                    shouldRestore = true;
                                }
                            } else if (vChar == KEY_O) {
                                if (data == KEY_T || data == KEY_W || data == KEY_L || data == KEY_E ||
                                    data == KEY_R || data == KEY_K || data == KEY_D || data == KEY_P || data == KEY_C || data == KEY_S || data == KEY_Q
                                    || (data == KEY_A && !repeatsMarkedVowel && !completesUa)
                                    || (data == KEY_O && !repeatsMarkedVowel && !completesUa)
                                    || (data == KEY_U && !hasPrecedingUw)
                                    || (data == KEY_Y && si == _index - 1)) {
                                    shouldRestore = true;
                                }
                            } else if (vChar == KEY_I) {
                                if (data == KEY_T || data == KEY_L || data == KEY_R || data == KEY_K || data == KEY_D || data == KEY_P || data == KEY_C || data == KEY_Q ||
                                    (data == KEY_E && !repeatsMarkedVowel) || (data == KEY_O && !completesUa) || data == KEY_I || data == KEY_Y) {
                                    shouldRestore = true;
                                }
                            } else {
                                if (data == KEY_T || data == KEY_L || data == KEY_R || data == KEY_K || data == KEY_D || data == KEY_P || data == KEY_C || data == KEY_Q ||
                                    (data == KEY_E && !repeatsMarkedVowel) || (data == KEY_A && !repeatsMarkedVowel && !completesUa) || (data == KEY_O && !repeatsMarkedVowel && !completesUa) ||
                                    (vChar == KEY_U && si == _index - 1 && !(TypingWord[si] & TONEW_MASK) && data == KEY_U)) {
                                    shouldRestore = true;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            // oo/ee/aa/w just changed the vowel. k, l, d, b, v, q, and a lone g
            // never end a Vietnamese syllable (book, look, good, tool, google, bowl).
            bool doubledVowel = _stateIndex >= 3
                && (KeyStates[_stateIndex - 3] & CHAR_MASK) == prevKey
                && (prevKey == KEY_A || prevKey == KEY_E || prevKey == KEY_O);
            bool justModifiedVowel = prevKey == KEY_W || doubledVowel;
            // dudowjc -> được. A later d still strikes đ when an earlier d is plain.
            // ddand -> dand. A later d cancels đ when initial d already has TONE_MASK.
            // good stays English: its d is not a second d.
            bool hasInitialD = _index >= 1 && CHR(0) == KEY_D;
            bool illegalCoda = data == KEY_K || data == KEY_L || data == KEY_B || data == KEY_V || data == KEY_Q
                || (data == KEY_W && !canWMod)
                || (data == KEY_Z && !hasAnyMark)
                || (data == KEY_D && !hasInitialD)
                || (data == KEY_G && !(_index >= 1 && CHR(_index - 1) == KEY_N));
            if (justModifiedVowel && illegalCoda)
                shouldRestore = true;
            // sojourn: sổ + u is not a syllable, so the raw word comes back.
            // thuajat: ậ + t is thuật, so t stays.
            if (!shouldRestore && _index >= 1
                && data != KEY_S && data != KEY_F && data != KEY_R && data != KEY_X
                && data != KEY_J && !(data == KEY_Z && hasAnyMark) && !canWMod && !completingCoda) {
                int vi = _index - 1;
                while (vi >= 0 && IS_CONSONANT(CHR(vi))) vi--;
                if (vi >= 0 && (TypingWord[vi] & TONE_MASK) && (TypingWord[vi] & MARK_MASK)) {
                    Uint16 v = CHR(vi);
                    bool ok = data == KEY_C || data == KEY_M || data == KEY_N || data == KEY_P || data == KEY_T;
                    if (v == KEY_A) ok = ok || data == KEY_U || data == KEY_Y;
                    if (v == KEY_E) ok = ok || data == KEY_U;
                    if (v == KEY_O) ok = ok || data == KEY_I;
                    if (!ok) shouldRestore = true;
                }
            }
            // Diphthongs ending in i or u (ai, oi, ui, au, iu, êu, âu, ôi, ơi, ươi) are already closed.
            // Once marked (bái in basic, múi in music, tải in tariff, vỉu in virus),
            // no consonant, non-repeating vowel, or second mark key after the glide can follow.
            // When circumflexed/horned (bôi in booing, bơi in bowie), no non-mark character can follow.
            if (!shouldRestore && !(data == KEY_Z && hasAnyMark) && _index >= 2) {
                Uint16 lastV = CHR(_index - 1);
                Uint16 prevV = CHR(_index - 2);
                bool isQu = _index >= 3 && CHR(_index - 3) == KEY_Q && prevV == KEY_U;
                bool isGi = _index >= 3 && CHR(_index - 3) == KEY_G && prevV == KEY_I;
                bool hasMarkOnDiphthong = ((TypingWord[_index - 2] | TypingWord[_index - 1]) & MARK_MASK) != 0;
                bool hasToneOnDiphthong = ((TypingWord[_index - 2] | TypingWord[_index - 1]) & (TONE_MASK | TONEW_MASK)) != 0;
                bool isMarkKey = data == KEY_S || data == KEY_F || data == KEY_R || data == KEY_X || data == KEY_J;
                bool isMarkOrMod = isMarkKey || canWMod || (data == KEY_D && CHR(0) == KEY_D);
                bool isTerminalAOrO = (lastV == KEY_A && ((prevV == KEY_U && ((TypingWord[_index - 2] & TONEW_MASK) || (!isQu && !hasToneOnDiphthong && prevKey == KEY_A && hasMarkOnDiphthong)))
                                                          || (prevV == KEY_I && !isGi)
                                                          || prevV == KEY_Y))
                    || (lastV == KEY_O && (prevV == KEY_A || prevV == KEY_E));
                if (!IS_CONSONANT(prevV) && (lastV == KEY_A || lastV == KEY_O) && !isQu && !isGi &&
                    hasMarkOnDiphthong && !hasVietMod && prevKey == lastV && isMarkKey) {
                    shouldRestore = true;
                } else if ((lastV == KEY_I || lastV == KEY_U) && !IS_CONSONANT(prevV) && !isQu && !isGi) {
                    if (hasMarkOnDiphthong) {
                        if (IS_CONSONANT(data) && !isMarkKey) {
                            shouldRestore = true;
                        } else if (!IS_CONSONANT(data) && !repeatsMarkedVowel) {
                            shouldRestore = true;
                        } else if (isMarkKey && prevKey == lastV && !hasVietMod &&
                                   ((((TypingWord[_index - 2] | TypingWord[_index - 1]) & MARK3_MASK) &&
                                     (data == KEY_S || data == KEY_F)) ||
                                    data == KEY_R)) {
                            shouldRestore = true;
                        }
                    } else if (hasToneOnDiphthong && !isMarkKey && !(data == prevV && (TypingWord[_index - 2] & TONE_MASK))) {
                        shouldRestore = true;
                    }
                } else if (isTerminalAOrO && (hasMarkOnDiphthong || hasToneOnDiphthong) && !isMarkOrMod) {
                    shouldRestore = true;
                }
            }
            // i never follows ê or â (seeing -> sê + i restores immediately; Vietnamese uses ây, never âi/êi).
            if (!shouldRestore && data == KEY_I && _index >= 1
                && (CHR(_index - 1) == KEY_E || CHR(_index - 1) == KEY_A)
                && (TypingWord[_index - 1] & TONE_MASK))
                shouldRestore = true;
            // The same consonants never follow a vowel at all (awful, hello, follow).
            if (!shouldRestore && _index >= 1 && !IS_CONSONANT(CHR(_index - 1)) && illegalCoda)
                shouldRestore = true;
            // A coda is already in place. Another letter starts a new word or is English
            // (owner, download, remember). Tone keys, w, and cancellation of â/ê/ô/đ still belong to this syllable.
            if (!shouldRestore && _index >= 2 && IS_CONSONANT(CHR(_index - 1)) && !completingCoda) {
                bool vowelBefore = false;
                bool cancelsCircumflex = false;
                bool hasStopCoda = CHR(_index - 1) == KEY_P || CHR(_index - 1) == KEY_T || CHR(_index - 1) == KEY_C ||
                    (CHR(_index - 1) == KEY_H && CHR(_index - 2) == KEY_C);
                bool hasAnyVowelMark = false;
                for (int si = 0; si < _index - 1; si++) {
                    if (!IS_CONSONANT(CHR(si))) {
                        vowelBefore = true;
                        if (TypingWord[si] & (MARK_MASK | TONE_MASK | TONEW_MASK)) {
                            hasAnyVowelMark = true;
                        }
                        if ((data == KEY_A || data == KEY_E || data == KEY_O) && CHR(si) == data && (TypingWord[si] & TONE_MASK)) {
                            if (!(hasStopCoda && data == KEY_E && !(TypingWord[si] & MARK_MASK))) {
                                cancelsCircumflex = true;
                            }
                        }
                    }
                }
                bool invalidMarkOnStopCoda = hasStopCoda && hasAnyVowelMark &&
                    (data == KEY_R || data == KEY_F || data == KEY_X);
                bool cancelsD = data == KEY_D && _index >= 1 && CHR(0) == KEY_D && (TypingWord[0] & TONE_MASK);
                bool toneOrHorn = (data == KEY_S || data == KEY_F || data == KEY_R || data == KEY_X
                    || data == KEY_J || (data == KEY_Z && hasAnyMark) || canWMod || cancelsCircumflex || cancelsD)
                    && !invalidMarkOnStopCoda;
                int deferredAt = -1;
                bool deferCircumflex = deferredCircumflex(false, data, deferredAt);
                if (vowelBefore && !toneOrHorn && !repeatVowelBeforeCoda && !deferCircumflex)
                    shouldRestore = true;
            }
            // e does not follow ơ, ă, ô, ư, â (tower, awesome, choose).
            // Plain o + e stays oe (khoẻ), and e after ê cancels circumflex (seeen -> seen).
            // Also if initial đ was formed by a delayed d (dude, diadem, dodge, dodrans), only tone/w/d (or o after du for được) can follow.
            if (!shouldRestore && _index >= 1 && CHR(0) == KEY_D && (TypingWord[0] & TONE_MASK) &&
                (_stateIndex < 2 || (KeyStates[1] & CHAR_MASK) != KEY_D)) {
                bool isDuoc = _index >= 2 && CHR(1) == KEY_U && (_index == 2 ? data == KEY_O : CHR(2) == KEY_O);
                bool okAfterDelayedD = data == KEY_S || data == KEY_F || data == KEY_R || data == KEY_X ||
                    data == KEY_J || (data == KEY_Z && hasAnyMark) || canWMod || data == KEY_D || isDuoc;
                if (!okAfterDelayedD)
                    shouldRestore = true;
            }
            if (!shouldRestore && data == KEY_E && _index >= 1
                && (!IS_CONSONANT(CHR(_index - 1)) && CHR(_index - 1) != KEY_E && (TypingWord[_index - 1] & (TONE_MASK | TONEW_MASK))))
                shouldRestore = true;
            // â is typed aa. a after ă is the English word await.
            if (!shouldRestore && data == KEY_A && _index >= 1
                && CHR(_index - 1) == KEY_A && (TypingWord[_index - 1] & TONEW_MASK))
                shouldRestore = true;
            // y here is the glide of ay/ây/uy/ăy (lấy). A following letter is a new word
            // (lawyer, async), except a tone key. u+y+e is uyê (khuyến, nguyễn).
            // thayas -> thấy: the second a of â sits after y.
            bool doublesVowelAfterY = false;
            if (_index >= 2 && CHR(_index - 1) == KEY_Y
                && (data == KEY_A || data == KEY_E || data == KEY_O || data == KEY_W)) {
                for (int si = _index - 2; si >= 0; si--) {
                    if (IS_CONSONANT(CHR(si)))
                        break;
                    if (data == KEY_W) {
                        if (CHR(si) == KEY_A || CHR(si) == KEY_O || CHR(si) == KEY_U) {
                            doublesVowelAfterY = true;
                            break;
                        }
                    } else if (CHR(si) == data && !(TypingWord[si] & TONE_MASK)) {
                        doublesVowelAfterY = true;
                        break;
                    }
                }
            }
            bool validAfterUy = CHR(_index - 2) == KEY_U && !(TypingWord[_index - 2] & TONEW_MASK)
                && (data == KEY_E || data == KEY_A || data == KEY_U || data == KEY_T
                    || data == KEY_P || data == KEY_N || data == KEY_C);
            if (!shouldRestore && _index >= 2 && CHR(_index - 1) == KEY_Y
                && !IS_CONSONANT(CHR(_index - 2))
                && !validAfterUy
                && !doublesVowelAfterY
                && data != KEY_S && data != KEY_F && data != KEY_R && data != KEY_X
                && data != KEY_J && !(data == KEY_Z && hasAnyMark))
                shouldRestore = true;
            // Standalone w at index 0:
            // - wh- (what, where, who), we- (we, web, well), wi- (win, wing, wife, wise, wizard),
            //   wo- (won, worm, work, word, world), wy-, and wr- (wrong, wrap, write, wrote) are English.
            // - wa followed by any non-tone letter (way, wait, want, warm, warn, water, watch, walk, wall) is English.
            if (!shouldRestore && _index >= 1 && CHR(0) == KEY_U && (TypingWord[0] & TONEW_MASK) && (TypingWord[0] & STANDALONE_MASK)) {
                bool isToneOrZ = data == KEY_S || data == KEY_F || data == KEY_R || data == KEY_X || data == KEY_J || (data == KEY_Z && hasAnyMark);
                if (_index == 1) {
                    if (data == KEY_H || data == KEY_E || data == KEY_I || data == KEY_O || data == KEY_Y ||
                        ((TypingWord[0] & MARK_MASK) && data == KEY_A)) {
                        shouldRestore = true;
                    }
                } else if (_index == 2 && CHR(1) == KEY_A) {
                    if (!isToneOrZ || ((TypingWord[0] | TypingWord[1]) & MARK_MASK)) {
                        shouldRestore = true;
                    }
                }
            }
            if (shouldRestore) {
                // If the screen already matches KeyStates[0.._index-1] with zero Vietnamese modifications,
                // pass the native key through (vDoNothing) instead of sending synthetic backspaces!
                bool alreadyPlain = (_index + 1 == _stateIndex);
                if (alreadyPlain) {
                    for (i = 0; i < _index; i++) {
                        if (TypingWord[i] != KeyStates[i] ||
                            (TypingWord[i] & (TONE_MASK | TONEW_MASK | MARK_MASK | STANDALONE_MASK))) {
                            alreadyPlain = false;
                            break;
                        }
                    }
                }
                if (alreadyPlain) {
                    insertKey(data, _isCaps, false);
                    tempDisableKey = true;
                    _spellingOK = true;
                    _spellingVowelOK = true;
                    hCode = vDoNothing;
                    hBPC = 0;
                    hNCC = 0;
                    hExt = 3;
                    if (vUseMacro) {
                        hMacroKey.push_back(data | (_isCaps ? CAPS_MASK : 0));
                    }
                    if (vUpperCaseFirstChar) {
                        if (_index == 1 && _upperCaseStatus == 2) {
                            upperCaseFirstCharacter();
                        }
                        _upperCaseStatus = 0;
                    }
                    return;
                }
                int commonPrefix = 0;
                int maxPrefix = (_index < _stateIndex) ? _index : _stateIndex;
                while (commonPrefix < maxPrefix &&
                       TypingWord[commonPrefix] == KeyStates[commonPrefix] &&
                       !(TypingWord[commonPrefix] & (TONE_MASK | TONEW_MASK | MARK_MASK | STANDALONE_MASK))) {
                    commonPrefix++;
                }
                hCode = vWillProcess;
                hBPC = _index - commonPrefix;
                hNCC = _stateIndex - commonPrefix;
                for (i = 0; i < _stateIndex; i++) {
                    TypingWord[i] = KeyStates[i];
                }
                for (i = commonPrefix; i < _stateIndex; i++) {
                    hData[_stateIndex - 1 - i] = TypingWord[i];
                }
                _index = _stateIndex;
                tempDisableKey = true;
                _spellingOK = true;
                _spellingVowelOK = true;
                if (vUseMacro) {
                    for (i = 0; i < hBPC; i++) {
                        if (hMacroKey.size() > 0)
                            hMacroKey.pop_back();
                    }
                    for (i = commonPrefix; i < _stateIndex; i++) {
                        hMacroKey.push_back(TypingWord[i]);
                    }
                }
                if (vUpperCaseFirstChar) {
                    if (_index == 1 && _upperCaseStatus == 2) {
                        upperCaseFirstCharacter();
                    }
                    _upperCaseStatus = 0;
                }
                return;
            }
        }
        
        if (!IS_SPECIALKEY(data) || tempDisableKey || _isInsideFileExtensionOrPath) { //do nothing
            if (!_isInsideFileExtensionOrPath && vQuickTelex && IS_QUICK_TELEX_KEY(data)) {
                handleQuickTelex(data, _isCaps);
            } else {
                hCode = vDoNothing;
                hBPC = 0;
                hNCC = 0;
                hExt = 3; //normal key
                insertKey(data, _isCaps);
            }
        } else { //check and update key
            //restore state
            hCode = vDoNothing;
            hExt = 3; //normal key
            handleMainKey(data, _isCaps);
        }

        if (!vFreeMark && !IS_KEY_D(data)) {
            if (hCode == vDoNothing) {
                checkGrammar(-1);
            } else {
                checkGrammar(0);
                // The extra vowel was already removed from the word. The screen
                // still has it, so delete one more character (vietej -> việt).
                if (deferredBackspace)
                    hBPC += deferredBackspace;
            }
        }
        
        _restoredOnPrevKey = false;
        if (hCode == vRestore) {
            // Normalize case of the earlier tone key press (ARrgon -> Argon).
            if (repeatTone && _stateIndex >= 2
                && (KeyStates[_stateIndex - 1] & CHAR_MASK) == data
                && (KeyStates[_stateIndex - 2] & CHAR_MASK) == data) {
                KeyStates[_stateIndex - 2] = KeyStates[_stateIndex - 1];
            }
            insertKey(data, _isCaps, false);
            bool isTripleRepeat = _stateIndex >= 3
                && (KeyStates[_stateIndex - 1] & CHAR_MASK) == data
                && (KeyStates[_stateIndex - 2] & CHAR_MASK) == data
                && (KeyStates[_stateIndex - 3] & CHAR_MASK) == data;
            if (isTripleRepeat) {
                // Triple consecutive key escape (coool -> cool, seeen -> seen, deeep -> deep):
                // drop the 3rd escape key from KeyStates so KeyStates has "cool"/"seen"/"deep".
                _stateIndex--;
            } else {
                _restoredOnPrevKey = true;
            }
        }
        
        //insert or replace key for macro feature
        if (vUseMacro) {
            if (hCode == vDoNothing) {
                hMacroKey.push_back(data | (_isCaps ? CAPS_MASK : 0));
            } else if (hCode == vWillProcess) {
                for (i = 0; i < hBPC; i++) {
                    if (hMacroKey.size() > 0) {
                        hMacroKey.pop_back();
                    }
                }
                for (i = _index - hNCC; i < _index; i++) {
                    if (i >= 0)
                        hMacroKey.push_back(TypingWord[i]);
                }
            } else if (hCode == vRestore) {
                for (i = 0; i < hBPC; i++) {
                    if (hMacroKey.size() > 0) {
                        hMacroKey.pop_back();
                    }
                }
                for (i = _index - 1 - hNCC; i < _index; i++) {
                    if (i >= 0)
                        hMacroKey.push_back(TypingWord[i]);
                }
            }
        }
        
        if (vUpperCaseFirstChar) {
            if (_index == 1 && _upperCaseStatus == 2) {
                upperCaseFirstCharacter();
            }
            _upperCaseStatus = 0;
        }
        
        //case [ ]
        if (IS_BRACKET_KEY(data) && (( IS_BRACKET_KEY((Uint16)hData[0])) || vInputType == vSimpleTelex1 || vInputType == vSimpleTelex2)) {
            if (_index - (hCode == vWillProcess ? hBPC : 0) > 0) {
                _index--;
                saveWord();
            }
            _index = 0;
            tempDisableKey = false;
            _stateIndex = 0;
            hExt = 3;
            _specialChar.push_back(data | (_isCaps ? CAPS_MASK : 0));
        }
    }
    
    //Debug
    //cout<<"index "<<(int)_index<< ", stateIndex "<<(int)_stateIndex<<", word "<<_typingStates.size()<<", long word "<<_longWordHelper.size()<< endl;
    //cout<<"backspace "<<(int)hBPC<<endl;
    //cout<<"new char "<<(int)hNCC<<endl<<endl;
}
