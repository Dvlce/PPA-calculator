#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <ctime>   // Per time_t, tm, localtime, mktime
#include <sstream> // Per stringstream
#include <limits>  // Per numeric_limits
#include <map>
#include <algorithm> // Per std::find_if

// Per Windows console API (colori, gotoxy) e output Unicode
#include <windows.h>
#include <io.h>
#include <fcntl.h>

using namespace std;

// --- Strutture Dati ---
struct DailyRecord {
    time_t date;       // Timestamp UNIX per la mezzanotte del giorno
    double amount;     // Importo del rendimento
    bool entered;      // Flag per distinguere 0.00 inserito da giorno saltato

    DailyRecord(time_t d, double a, bool e) : date(d), amount(a), entered(e) {}
};

struct UserProfile {
    double salary;
    vector<DailyRecord> records;
    string dataFilePath;

    UserProfile(string path) : salary(0.0), dataFilePath(std::move(path)) {}
};

// --- Funzioni Console ---
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void clearScreen() {
    // Una semplice implementazione per pulire lo schermo stampando nuove righe
    // o usando system("cls") se preferito (ma meno portabile/pulito)
    // Per ora, ci affideremo al ridisegno e a gotoxy per sovrascrivere.
    // system("cls"); // Scommentare se necessario, ma può causare sfarfallio
}

void drawLine(int length, wchar_t charToDraw = L'-') {
    for (int i = 0; i < length; i++) wcout << charToDraw;
    wcout << endl;
}


// --- Funzioni Titolo Pixel Art (Simile al precedente) ---
const int LETTER_HEIGHT_TITLE = 5;
const wstring PIXEL_CHAR_TITLE = L"█";
const wstring EMPTY_PIXEL_CHAR_TITLE = L" ";
const wstring INTER_LETTER_SPACE_TITLE = L"  "; // Due spazi per distinguere meglio

const char* D_PATTERN[LETTER_HEIGHT_TITLE] = {"@@ ", "@ @", "@ @", "@ @", "@@ "};
const char* A_PATTERN_TITLE[LETTER_HEIGHT_TITLE] = {"@@@", "@ @", "@@@", "@ @", "@ @"};
const char* T_PATTERN_TITLE[LETTER_HEIGHT_TITLE] = {"@@@", " @ ", " @ ", " @ ", " @ "};
const char* R_PATTERN_TITLE[LETTER_HEIGHT_TITLE] = {"@@@", "@ @", "@@ ", "@ @", "@ @"};
const char* E_PATTERN_TITLE[LETTER_HEIGHT_TITLE] = {"@@@", "@  ", "@@ ", "@  ", "@@@"};
const char* G_PATTERN[LETTER_HEIGHT_TITLE] = {" @@", "@  ", "@ @@", "@  @", " @@"};
const char* I_PATTERN[LETTER_HEIGHT_TITLE] = {"@@@", " @ ", " @ ", " @ ", "@@@"};
const char* S_PATTERN[LETTER_HEIGHT_TITLE] = {" @@", "@  ", " @ ", "  @", "@@ "};


map<char, const char**> titleLetterPatterns = {
    {'D', D_PATTERN}, {'A', A_PATTERN_TITLE}, {'T', T_PATTERN_TITLE},
    {'R', R_PATTERN_TITLE}, {'E', E_PATTERN_TITLE}, {'G', G_PATTERN},
    {'I', I_PATTERN}, {'S', S_PATTERN}
};

void printPixelArtWordTitle(const string& word, int singleColor, int consoleWidth, int startY) {
    if (word.empty()) return;
    int totalCharWidth = 0;
    for (size_t k = 0; k < word.length(); ++k) {
        char c = toupper(word[k]);
        if (titleLetterPatterns.count(c)) {
            totalCharWidth += strlen(titleLetterPatterns[c][0]);
        }
        if (k < word.length() - 1) {
            totalCharWidth += INTER_LETTER_SPACE_TITLE.length();
        }
    }
    int padding = (consoleWidth - totalCharWidth) / 2;
    if (padding < 0) padding = 0;

    for (int i = 0; i < LETTER_HEIGHT_TITLE; ++i) {
        gotoxy(0, startY + i);
        wcout << wstring(consoleWidth, L' '); // Pulisce la riga

        gotoxy(padding, startY + i);
        for (size_t j = 0; j < word.length(); ++j) {
            char currentChar = toupper(word[j]);
            if (titleLetterPatterns.count(currentChar)) {
                const char** pattern = titleLetterPatterns[currentChar];
                string lineSegment = pattern[i];
                for (char pixel_char_in_pattern : lineSegment) {
                    if (pixel_char_in_pattern == '@') {
                        setColor(singleColor);
                        wcout << PIXEL_CHAR_TITLE;
                    } else {
                        wcout << EMPTY_PIXEL_CHAR_TITLE;
                    }
                }
            }
            if (j < word.length() - 1) {
                wcout << INTER_LETTER_SPACE_TITLE;
            }
        }
    }
    setColor(7); // Reset
}

void displayProgramTitle(int consoleWidth) {
    int titleColor = 11; // Ciano chiaro
    int line1_Y = 1;
    int line2_Y = line1_Y + LETTER_HEIGHT_TITLE + 1;

    printPixelArtWordTitle("DATA", titleColor, consoleWidth, line1_Y);
    printPixelArtWordTitle("REGISTER", titleColor, consoleWidth, line2_Y);
    
    gotoxy(0, line2_Y + LETTER_HEIGHT_TITLE +1); // Posiziona cursore dopo il titolo
}


// --- Funzioni Data/Ora ---
time_t getMidnightTimestamp(time_t ts) {
    tm ltm = *localtime(&ts);
    ltm.tm_hour = 0;
    ltm.tm_min = 0;
    ltm.tm_sec = 0;
    return mktime(&ltm);
}

time_t getCurrentDateMidnight() {
    time_t now = time(0);
    return getMidnightTimestamp(now);
}

// Ottiene il timestamp per l'inizio della settimana (Lunedì) per una data data
time_t getStartOfWeek(time_t date) {
    tm ltm = *localtime(&date);
    // tm_wday è 0 per Domenica, 1 per Lunedì, ..., 6 per Sabato
    int daysToSubtract = ltm.tm_wday - 1; // Giorni da sottrarre per arrivare a Lunedì
    if (ltm.tm_wday == 0) { // Se è Domenica (0), vai indietro di 6 giorni
        daysToSubtract = 6;
    }
    ltm.tm_mday -= daysToSubtract;
    ltm.tm_hour = 0; ltm.tm_min = 0; ltm.tm_sec = 0;
    return mktime(&ltm);
}

// Ottiene il timestamp per l'inizio del mese
time_t getStartOfMonth(time_t date) {
    tm ltm = *localtime(&date);
    ltm.tm_mday = 1;
    ltm.tm_hour = 0; ltm.tm_min = 0; ltm.tm_sec = 0;
    return mktime(&ltm);
}

int getDaysInMonth(int month_1_12, int year) {
    if (month_1_12 == 2) {
        return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
    } else if (month_1_12 == 4 || month_1_12 == 6 || month_1_12 == 9 || month_1_12 == 11) {
        return 30;
    }
    return 31;
}

wstring formatDate(time_t date, const wchar_t* fmt = L"%d/%m/%Y") {
    wchar_t buffer[80];
    tm ltm = *localtime(&date);
    wcsftime(buffer, sizeof(buffer), fmt, &ltm);
    return buffer;
}

wstring getDayName(int dayOfWeek_0_6, bool brief = false) { // 0=Domenica, 1=Lunedì...
    // Attenzione: tm_wday è 0 per Domenica. Adattiamo per Lun=0, Mar=1... Dom=6 per la tabella
    // Ma la funzione qui prende 0 per Domenica come da tm_wday
    const wchar_t* daysFull[] = {L"Domenica", L"Lunedì", L"Martedì", L"Mercoledì", L"Giovedì", L"Venerdì", L"Sabato"};
    const wchar_t* daysBrief[] = {L"Dom", L"Lun", L"Mar", L"Mer", L"Gio", L"Ven", L"Sab"};
    if (dayOfWeek_0_6 >= 0 && dayOfWeek_0_6 <= 6) {
        return brief ? daysBrief[dayOfWeek_0_6] : daysFull[dayOfWeek_0_6];
    }
    return L"N/A";
}


// --- Funzioni File I/O ---
bool loadProfile(UserProfile& profile) {
    wifstream file(profile.dataFilePath);
    if (!file.is_open()) return false;

    wstring line;
    while (getline(file, line)) {
        wstringstream ss(line);
        wstring key;
        getline(ss, key, L':');

        if (key == L"SALARY") {
            ss >> profile.salary;
        } else if (key == L"RECORD") {
            time_t date_ts;
            double amount_val;
            bool entered_val = true; // Assumiamo true se il record esiste
            wchar_t sep; // per il secondo ':'

            ss >> date_ts >> sep >> amount_val;
            // Per distinguere uno 0.0 inserito da un giorno saltato, potremmo aggiungere un flag al file
            // Per ora, se amount è 0.0, lo trattiamo come inserito.
            // La logica di "giorno saltato" avverrà in visualizzazione se manca un record.
            profile.records.push_back(DailyRecord(date_ts, amount_val, entered_val));
        }
    }
    file.close();
    // Ordina i record per data per sicurezza, sebbene dovrebbero essere aggiunti in ordine
    sort(profile.records.begin(), profile.records.end(), [](const DailyRecord& a, const DailyRecord& b){
        return a.date < b.date;
    });
    return true;
}

bool saveProfile(const UserProfile& profile) {
    wofstream file(profile.dataFilePath);
    if (!file.is_open()) {
        wcerr << L"Errore: Impossibile aprire il file per il salvataggio: " << profile.dataFilePath.c_str() << endl;
        return false;
    }

    file << L"SALARY:" << fixed << setprecision(2) << profile.salary << endl;
    for (const auto& record : profile.records) {
        if (record.entered) { // Salva solo i record effettivamente inseriti
             file << L"RECORD:" << record.date << L":" << fixed << setprecision(2) << record.amount << endl;
        }
    }
    file.close();
    return true;
}

// --- Logica Principale dell'Applicazione ---
void inputDailyRendiment(UserProfile& profile) {
    time_t todayMidnight = getCurrentDateMidnight();

    auto it = find_if(profile.records.begin(), profile.records.end(), 
                      [todayMidnight](const DailyRecord& rec){ return rec.date == todayMidnight && rec.entered; });

    if (it != profile.records.end()) {
        wcout << L"Rendimento per oggi (" << formatDate(todayMidnight, L"%d/%m") << L") già inserito: ";
        setColor(it->amount > 0 ? 10 : (it->amount < 0 ? 12 : 9)); // Verde, Rosso, Blu
        wcout << fixed << setprecision(2) << it->amount << L" EUR" << endl;
        setColor(7);
        return;
    }

    wcout << L"\nInserisci il rendimento per oggi (" << formatDate(todayMidnight, L"%A %d %B %Y") << L"):" << endl;
    wcout << L"Importo (es. 50.25 per positivo, -15.50 per negativo, 0 per neutro): ";
    double amount;
    while (!(wcin >> amount)) {
        wcout << L"Input non valido. Inserisci un numero: ";
        wcin.clear();
        wcin.ignore(numeric_limits<streamsize>::max(), L'\n');
    }
    wcin.ignore(numeric_limits<streamsize>::max(), L'\n'); // Pulisce il buffer di input

    profile.records.push_back(DailyRecord(todayMidnight, amount, true));
    // Rimuovi eventuali record "non inseriti" per oggi, se ce ne fossero (non dovrebbe accadere con la logica attuale)
    profile.records.erase(remove_if(profile.records.begin(), profile.records.end(),
        [todayMidnight](const DailyRecord& rec){ return rec.date == todayMidnight && !rec.entered; }), profile.records.end());

    saveProfile(profile); // Salva subito dopo l'inserimento
    wcout << L"Rendimento salvato: " << fixed << setprecision(2) << amount << L" EUR." << endl;
}


void displayWeeklyReport(const UserProfile& profile) {
    wcout << L"\n--- RENDIMENTO SETTIMANALE (" << formatDate(getCurrentDateMidnight(), L"%B %Y") << L") ---" << endl;
    drawLine(70);
    wcout << left << setw(15) << L"Giorno" << setw(20) << L"Data" << setw(15) << L"Rendimento (EUR)" << endl;
    drawLine(70);

    time_t startOfWeek = getStartOfWeek(getCurrentDateMidnight());
    tm ltmStart = *localtime(&startOfWeek);

    for (int i = 0; i < 7; ++i) { // Da Lunedì (0) a Domenica (6)
        tm currentDayTm = ltmStart;
        currentDayTm.tm_mday += i; // Aggiunge i giorni a partire da Lunedì
        time_t currentDayTs = getMidnightTimestamp(mktime(&currentDayTm)); // Normalizza e ottieni timestamp a mezzanotte

        // Nome del giorno (tm_wday è 0 per Dom, 1 Lun... quindi (ltmStart.tm_wday + i -1 + 7)%7 per Lun=0...Dom=6)
        // Più semplicemente, usiamo la data corrente
        tm displayDayTm = *localtime(&currentDayTs);
        wstring dayNameStr = getDayName(displayDayTm.tm_wday);


        wcout << left << setw(15) << dayNameStr;
        wcout << setw(20) << formatDate(currentDayTs, L"%d/%m/%Y");

        auto it = find_if(profile.records.begin(), profile.records.end(),
                          [currentDayTs](const DailyRecord& rec){ return rec.date == currentDayTs && rec.entered; });
        
        double amountToShow = 0.0;
        bool wasEntered = false;
        if (it != profile.records.end()) {
            amountToShow = it->amount;
            wasEntered = true;
        }

        int color = 9; // Blu per neutro/non inserito
        if (wasEntered) {
            if (amountToShow > 0.001) color = 10; // Verde
            else if (amountToShow < -0.001) color = 12; // Rosso
        }
        
        setColor(color);
        wcout << right << setw(10) << fixed << setprecision(2) << amountToShow;
        setColor(7);
        wcout << (wasEntered ? L"" : L" (non inserito)") << endl;
    }
    drawLine(70);
}

void displayMonthlyReport(const UserProfile& profile) {
    time_t today = getCurrentDateMidnight();
    tm todayTm = *localtime(&today);
    int currentMonth = todayTm.tm_mon + 1; // 1-12
    int currentYear = todayTm.tm_year + 1900;

    wcout << L"\n--- RENDIMENTO MENSILE (" << formatDate(today, L"%B %Y") << L") ---" << endl;
    drawLine(70);
    wcout << left << setw(20) << L"Periodo Settimana" << setw(25) << L"Rendimento Totale (EUR)" << endl;
    drawLine(70);

    int daysInCurrentMonth = getDaysInMonth(currentMonth, currentYear);
    
    // Definisci i periodi settimanali
    vector<pair<int, int>> week_periods;
    week_periods.push_back({1, 7});
    week_periods.push_back({8, 14});
    week_periods.push_back({15, 21});
    week_periods.push_back({22, daysInCurrentMonth}); // L'ultima "settimana" copre fino a fine mese

    for (int i = 0; i < week_periods.size(); ++i) {
        int startDay = week_periods[i].first;
        int endDay = week_periods[i].second;
        if (startDay > daysInCurrentMonth) continue; // Se il mese è corto (es. Febbraio)
        if (endDay > daysInCurrentMonth) endDay = daysInCurrentMonth;


        double weeklyTotal = 0;
        tm weekStartTm = todayTm; weekStartTm.tm_mday = startDay;
        time_t periodStartTs = getMidnightTimestamp(mktime(&weekStartTm));

        tm weekEndTm = todayTm; weekEndTm.tm_mday = endDay;
        time_t periodEndTs = getMidnightTimestamp(mktime(&weekEndTm));


        for (const auto& record : profile.records) {
            if (record.entered && record.date >= periodStartTs && record.date <= periodEndTs) {
                weeklyTotal += record.amount;
            }
        }
        
        wstringstream periodLabel;
        periodLabel << L"Sett. " << (i + 1) << L" (Gg. " << startDay << L"-" << endDay << L")";
        wcout << left << setw(20) << periodLabel.str();

        int color = 9; // Blu
        if (weeklyTotal > 0.001) color = 10; // Verde
        else if (weeklyTotal < -0.001) color = 12; // Rosso
        
        setColor(color);
        wcout << right << setw(20) << fixed << setprecision(2) << weeklyTotal << endl;
        setColor(7);
    }
    drawLine(70);
}


int main() {
    _setmode(_fileno(stdout), _O_U16TEXT); // Per output Unicode
    _setmode(_fileno(stdin),  _O_U16TEXT); // Per input Unicode (wcin)
    // setlocale(LC_ALL, "it_IT.UTF-8"); // Potrebbe essere utile per formattazione date/numeri, ma _setmode è più per i caratteri

    UserProfile profile("dataregister_data.txt");
    int consoleWidth = 80;

    if (!loadProfile(profile)) {
        wcout << L"Benvenuto in Data Register! Sembra sia la prima volta." << endl;
        wcout << L"Inserisci il tuo salario mensile di base (EUR): ";
        while (!(wcin >> profile.salary) || profile.salary < 0) {
            wcout << L"Input non valido. Inserisci un numero positivo per il salario: ";
            wcin.clear();
            wcin.ignore(numeric_limits<streamsize>::max(), L'\n');
        }
        wcin.ignore(numeric_limits<streamsize>::max(), L'\n');
        saveProfile(profile); // Salva il profilo iniziale con il salario
    } else {
        wcout << L"Bentornato! Salario attuale: " << fixed << setprecision(2) << profile.salary << L" EUR." << endl;
    }

    bool running = true;
    while(running) {
        clearScreen(); // Pulisce lo schermo (implementazione base)
        gotoxy(0,0);   // Torna all'inizio della console
        
        displayProgramTitle(consoleWidth); // Mostra il titolo
        
        // Mostra sempre il salario attuale sotto il titolo o da qualche parte fisso
        gotoxy(0, LETTER_HEIGHT_TITLE * 2 + 4); // Posiziona sotto il titolo
        wcout << L"Salario Base Registrato: " << fixed << setprecision(2) << profile.salary << L" EUR" << endl;
        drawLine(consoleWidth, L'=');


        inputDailyRendiment(profile); // Chiede il rendimento per oggi
        displayWeeklyReport(profile);
        displayMonthlyReport(profile);

        wcout << L"\nCosa vuoi fare?" << endl;
        wcout << L"1. Aggiorna/Inserisci rendimento per oggi (già fatto se non mostrato sopra)" << endl;
        wcout << L"2. Modifica Salario Base" << endl;
        wcout << L"3. Visualizza di nuovo i report" << endl;
        wcout << L"4. Esci" << endl;
        wcout << L"Scelta: ";

        int choice;
        if (!(wcin >> choice)) {
            wcin.clear();
            wcin.ignore(numeric_limits<streamsize>::max(), L'\n');
            choice = 0; // Scelta non valida
        } else {
            wcin.ignore(numeric_limits<streamsize>::max(), L'\n'); // Consuma il newline
        }


        switch (choice) {
            case 1:
                // La funzione inputDailyRendiment è già chiamata all'inizio del loop,
                // quindi questa opzione è più per un refresh o se l'utente vuole forzare.
                // Per ora, la logica principale la gestisce già.
                wcout << L"Il rendimento odierno viene richiesto automaticamente all'avvio." << endl;
                // Potremmo aggiungere una logica per inserire/modificare rendimenti di giorni passati qui.
                break;
            case 2: {
                wcout << L"Inserisci il nuovo salario mensile di base (EUR): ";
                double newSalary;
                 while (!(wcin >> newSalary) || newSalary < 0) {
                    wcout << L"Input non valido. Inserisci un numero positivo: ";
                    wcin.clear();
                    wcin.ignore(numeric_limits<streamsize>::max(), L'\n');
                }
                wcin.ignore(numeric_limits<streamsize>::max(), L'\n');
                profile.salary = newSalary;
                saveProfile(profile);
                wcout << L"Salario aggiornato a " << fixed << setprecision(2) << profile.salary << L" EUR." << endl;
                break;
            }
            case 3:
                // I report vengono già visualizzati, questo forza un "refresh" della schermata.
                // La pulizia e il ridisegno avvengono all'inizio del loop.
                wcout << L"Report aggiornati." << endl;
                break;
            case 4:
                running = false;
                wcout << L"Arrivederci!" << endl;
                break;
            default:
                wcout << L"Scelta non valida. Riprova." << endl;
                break;
        }
        if (running) {
             wcout << L"\nPremi INVIO per continuare...";
             wcin.get(); // Attende invio
        }
    }

    return 0;
}

