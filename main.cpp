#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <cstdio>

//#define byte int8_t;

using namespace std;


string readStr4(ifstream &in) {
    string r = "";
    for (int i = 0; i < 4; ++i)
        r += char(in.get());
    return r;
}

uint32_t readInt(ifstream &in) {
    char buffer[4];
    for (int i = 0; i < 4; ++i)
        in.get(buffer[i]);
    return uint32_t((unsigned char) (buffer[0]) << 24 |
                    (unsigned char) (buffer[1]) << 16 |
                    (unsigned char) (buffer[2]) << 8 |
                    (unsigned char) (buffer[3]));
}

uint16_t readShort(ifstream &in) {
    char buffer[2];
    for (int i = 0; i < 2; ++i)
        in.get(buffer[i]);
    return uint16_t((unsigned char) (buffer[0]) << 8 | (unsigned char) (buffer[1]));
}


// Назначение: копирование блока MTrk (блок с событиями) из MIDI файла.
// Параметры: поток для чтения MIDI файла.
// Возвращает: структуру блока с массивом структур событий.
void readDataBlock(ifstream &in) {
    string nameSection = readStr4(in);
    uint32_t lengthSection = readInt(in); // 4 байта длинны всего блока.
    uint32_t LoopIndex = lengthSection; // Копируем колличество оставшихся ячеек. Будем считывать события, пока счетчик не будет = 0.
    uint32_t realTime = 0; // Реальное время внутри блока.
    while (LoopIndex != 0) // Пока не считаем все события.
    {


        // Время описывается плавающим числом байт. Конечный байт не имеет 8-го разрядка справа (самого старшего).
        int loopСount = 0; // Колличество считанных байт.
        uint8_t buffer; // Сюда кладем считанное значение.
        uint32_t bufferTime = 0; // Считанное время помещаем сюда.
        do {
            buffer = in.get(); // Читаем значение.
            loopСount++; // Показываем, что считали байт.
            bufferTime <<= 7; // Сдвигаем на 7 байт влево существующее значенеи времени (Т.к. 1 старший байт не используется).
            bufferTime |= uint8_t(
                    uint8_t(buffer) & uint8_t(0x7F)); // На сдвинутый участок накладываем существующее время.
        } while ((buffer & (1 << 7)) != 0); // Выходим, как только прочитаем последний байт времени (старший бит = 0).
        realTime += bufferTime; // Получаем реальное время.

        buffer = in.get();
        loopСount++; // Считываем статус-байт, показываем, что считали байт.
        // Если у нас мета-события, то...
        if (buffer == 0xFF) {
            buffer = in.get();  // Считываем номер мета-события.
            buffer = in.get();  // Считываем длину.
            loopСount += 2;
            for (int loop = 0; loop < buffer; loop++)
                in.get();
            LoopIndex = LoopIndex - loopСount - buffer; // Отнимаем от счетчика длинну считанного.
        }

            // Если не мета-событие, то смотрим, является ли событие событием первого уровня.
        else
            switch ((int8_t) buffer & 0xF0) // Смотрим по старшым 4-м байтам.
            {
                // Перебираем события первого уровня.

                case 0x80: // Снять клавишу.
                    bufferSTNote.channelNote = (byte) (buffer & 0x0F); // Копируем номер канала.
                    bufferSTNote.flagNote = false; // Мы отпускаем клавишу.
                    bufferSTNote.roomNotes = MIDIFile.ReadByte(); // Копируем номер ноты.
                    bufferSTNote.dynamicsNote = MIDIFile.ReadByte(); // Копируем динамику ноты.
                    bufferSTNote.noteTime = realTime; // Присваеваем реальное время ноты.
                    ST.arrayNoteStruct.Add(bufferSTNote); // Сохраняем новую структуру.
                    LoopIndex = LoopIndex - loopСount - 2; // Отнимаем прочитанное.
                    break;
                case 0x90:   // Нажать клавишу.
                    bufferSTNote.channelNote = (byte) (buffer & 0x0F); // Копируем номер канала.
                    bufferSTNote.flagNote = true; // Мы нажимаем.
                    bufferSTNote.roomNotes = MIDIFile.ReadByte(); // Копируем номер ноты.
                    bufferSTNote.dynamicsNote = MIDIFile.ReadByte(); // Копируем динамику ноты.
                    bufferSTNote.noteTime = realTime; // Присваеваем реальное время ноты.
                    ST.arrayNoteStruct.Add(bufferSTNote); // Сохраняем новую структуру.
                    LoopIndex = LoopIndex - loopСount - 2; // Отнимаем прочитанное.
                    break;
                case 0xA0:  // Сменить силу нажатия клавишы.
                    bufferSTNote.channelNote = (byte) (buffer & 0x0F); // Копируем номер канала.
                    bufferSTNote.flagNote = true; // Мы нажимаем.
                    bufferSTNote.roomNotes = MIDIFile.ReadByte(); // Копируем номер ноты.
                    bufferSTNote.dynamicsNote = MIDIFile.ReadByte(); // Копируем НОВУЮ динамику ноты.
                    bufferSTNote.noteTime = realTime; // Присваеваем реальное время ноты.
                    ST.arrayNoteStruct.Add(bufferSTNote); // Сохраняем новую структуру.
                    LoopIndex = LoopIndex - loopСount - 2; // Отнимаем прочитанное.
                    break;
                    // Если 2-х байтовая комманда.
                case 0xB0:
                    byte buffer2level = MIDIFile.ReadByte(); // Читаем саму команду.
                    switch (buffer2level) // Смотрим команды второго уровня.
                    {
                        default: // Для определения новых комманд (не описаных).
                            MIDIFile.ReadByte(); // Считываем параметр какой-то неизвестной функции.
                            LoopIndex = LoopIndex - loopСount - 2; // Отнимаем прочитанное.
                            break;
                    }
                    break;

                    // В случае попадания их просто нужно считать.
                case 0xC0:   // Просто считываем байт номера.
                    MIDIFile.ReadByte(); // Считываем номер программы.
                    LoopIndex = LoopIndex - loopСount - 1; // Отнимаем прочитанное.
                    break;

                case 0xD0:   // Сила канала.
                    MIDIFile.ReadByte(); // Считываем номер программы.
                    LoopIndex = LoopIndex - loopСount - 1; // Отнимаем прочитанное.
                    break;

                case 0xE0:  // Вращения звуковысотного колеса.
                    MIDIFile.ReadBytes(2); // Считываем номер программы.
                    LoopIndex = LoopIndex - loopСount - 2; // Отнимаем прочитанное.
                    break;
            }
    }
    return ST; // Возвращаем заполненную структуру.
}

void loadMidi(string filename) {
    ifstream in(filename);

    int infoSize, fileType, blockCnt, timeFormat;

    infoSize = readInt(in);
    fileType = readShort(in);
    blockCnt = readShort(in);
    timeFormat = readShort(in);
    char c;
    while ((c = in.get()) != EOF) {
        cout << hex << c + 0 << ' ';
    }
    cout << "\nEOF\n";
    in.close();


}


int main() {
    //loadMidi("test1.mid");
    loadMidi("D:\\Programming\\VS_CPP\\RandomThings\\midiReader\\test1.mid");


    return 0;
}
