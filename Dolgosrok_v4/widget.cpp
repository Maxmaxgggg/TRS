#include "widget.h"
#include "ui_widget.h"
#include <QTime>
#include <QFileDialog>
#include <cmath>
#define USE_MATH_DEFINES

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->in_FilePBN->setProperty( "fileRole", 100 );
    ui->outFilePBN->setProperty( "fileRole", 101 );

    buf = new float[BUF_SIZE];
    filter = new RealFIR( RealFIR::Bandpass,
                          CARRIER_FRQ - 1000,
                          CARRIER_FRQ + 1000 );

    log( "Приложение запущено ");
}

Widget::~Widget()
{
    clearAlphabet();
    delete      ui;
    delete[]    buf;
}

                                                            /*      МЕТОДЫ       */


// Метод вывода на экран действий пользователя
void Widget::log(const QString &s)
{
    QString m;
    m = QTime::currentTime().toString();
    m += "    ";
    m += s;
    ui->logPTE->appendPlainText( m );
    qDebug() << m;
}

// Метод очистки строки от символ не в кодировке ASCII7
Result Widget::rmNonASCII(const QString &input)
{
    QString res;
    int nonASCII = 0;
    // Удаляеем все символы из строки не в кодировке ASCII7
    for ( QChar ch : input ) {
        if ( ch.unicode() < 128 ) {
            res.append( ch );
        } else {
            nonASCII++;
        }
    }

    return {res, nonASCII};
}

// Метод преобразования исходной строки в массив уровней
QVector<int> Widget::cvrtToBit(const QString &input)
{

    int             asciiValue; // Переменная, хранящая ASCII7 код отдельного символа

    int             value;      // Переменная, хранящая уровень отдельно взятых двух бит ( их десятичное представление )

    QString         bitPart;    // Переменная, хранящая двоичное представление отдельного символа

    QString         bitString;  // Переменная, хранящая двоичное представление всего сообщения

    QString         bitPair;    // Переменная, хранящая два бита исходного сообщения

    QVector<int>    result;     // Вектор, хранящий уровни сигнала ( от 0-го до 3-го )


    // Преобразуем каждый символ в 7-битный формат
    for (QChar ch : input) {
        asciiValue  = ch.unicode();
        bitPart     = QString("%1").arg(asciiValue, 7, 2, QChar('0'));
        bitString.append(bitPart);
    }

    // Если число бит нечетное, то добавляем в конец '0'
    if( bitString.size() % 2 != 0 )
        bitString.append('0');


    // Разбиваем строку битов на группы по 2 бита и преобразуем каждую группу в число от 0 до 3
    for (int i = 0; i < bitString.size(); i += 2) {
        bitPair = bitString.mid(i, 2);
        value = bitPair.toInt(nullptr, 2);
        result.append( value );
    }

    return result;
}

// Метод добавления символов 3,2,1,0 в алфавит
void Widget::addSymbol( int    c )
{
    if( alphabet.contains(c) ) {
        delete alphabet[c];
        alphabet.remove(c);
    }

    alphabet[c] = new Symbol( S_SIZE );
    float amp = CARRIER_AMP;
    try {
        switch ( c ) {

        case 3: amp*=3;  break;
        case 2:          break;
        case 1: amp*=-1; break;
        case 0: amp*=-3; break;

        default: throw("Неизвестный символ"); break;
        }
    }
    catch( const char* s ) {
        log(s);
    }
    for ( int i = 0; i < S_SIZE; ++i )
        alphabet[c]->data[i] = amp*cos( 2*M_PI*CARRIER_FRQ/SMPL_FRQ*i );

}

// Метод очистки алфавита
void Widget::clearAlphabet()
{
    foreach( Symbol *s, alphabet){
        delete s;
    }

    alphabet.clear();
}


                                                            /*      СЛОТЫ       */


void Widget::fileSelectSlot()
{
    QObject     *obj = sender();

    if( obj != NULL ) {
        QLineEdit   *led = NULL;
        QString     str;

        int         role = obj->property("fileRole").toInt();
        switch( role ){
        case 100 :
            led = ui->in_FileLED;
            str = "исходный файл";
            break;
        case 101 :
            led = ui->outFileLED;
            str = "выходной файл";
            break;
        };
        if( led != NULL ) {
            log( "Выбираем " + str );
            QString fileName = QFileDialog::getOpenFileName(
                this,
                "Выберите " + str,
                "D:/");
            if( fileName.isEmpty() ) {
                log( "Пользователь отказался от выбора файла" );
            } else {
                led->setText( fileName );
            }
        } else {
            log( "Вызывающий объект неизвестен" );
        }
    }
}

void Widget::on_progressPBN_clicked()
{
    try {

        log( "Обработка запущена" );

        // Добавляем уровни сигнала в алфавит
        // 3 <-> +3, 2 <-> +1, 1 <-> -1, 0 <-> -3
        clearAlphabet();
        addSymbol( 0 );
        addSymbol( 1 );
        addSymbol( 2 );
        addSymbol( 3 );

                /*          Определяем переменные           */

        QString         text;           // Исходное сообщение пользователя

        QString         textASCII;      // Сообщение пользователя, где удалены символы в неправильной кодировке

        int             nonASCII = 0;   // Число удаленных символов

        Result          res;            // Объект, содержащий строку и число удаленных символов

        QVector<int>    ampVec;         // Вектор, в котором хранятся значения сигнала прямоугольных радиоимпульсов


                /*          Получаем исходное сообщение пользователя         */

        // Если пользователь ввел текст
        if( !ui->mesLED->text().isEmpty() )
           text = ui->mesLED->text();
        // Если пользователь выбрал файл
        if( !ui->in_FileLED->text().isEmpty() ) {
            // Если пользователь и выбрал файл, и ввел сообщение, то приоритет отдается файлу
            if( !text.isEmpty())
                log( "Пользователь и выбрал файл, и ввел сообщение."
                     "Приоритет отдается файлу" );

            QString fileName = ui->in_FileLED->text();
            QFile       inFile(fileName);
            if( inFile.open(QIODevice::ReadOnly) ) {
                // Создаем объект QTextStream для чтения из файла
                QTextStream in(&inFile);

                // Читаем все содержимое файла в QString
                text = in.readAll();

                // Закрываем файл
                inFile.close();
            } else {
                throw( "Ошибка при открытии исходного файла" );
            }
        }
        if( ui->mesLED->text().isEmpty() && ui->in_FileLED->text().isEmpty() )
            throw( "Ошибка, пользователь не выбрал файл и не ввел сообщение" );

                /*          Получаем вектор амплитуд импульсов              */

        log( "Приступаем к обработке данных" );

        // Убираем все символы не в кодировке ASCII7
        res         = rmNonASCII( text );
        textASCII   = res.textASCII;
        nonASCII    = res.nonASCII;
        ampVec      = cvrtToBit(textASCII);


        if( nonASCII != 0 ) {
            log( "В сообщении/файле обнаружено " + QString::number( nonASCII ) +
                " символов не в кодировке ASCII7. Они будут пропущены" );
        }


                /*          Записываем результат в выходной файл            */


        QFile outFile ( ui->outFileLED->text());
        if( !outFile.open(QIODevice::WriteOnly) ) {
            throw( "Ошибка при открытии выходного файла" );
        }

        outFile.resize(0);
        int         fltSize;
        foreach (int c, ampVec) {
            filter->filter( alphabet[c]->data,
                            alphabet[c]->size,
                            buf,
                            &fltSize );
            outFile.write( (char*) buf, fltSize*sizeof(float) );
            /*outFile.write( (char*) alphabet[c]->data, alphabet[c]->size*sizeof(float) );*/
        }
        log("Обработка завершена. Результат сохранен в выходной файл");
    }

        /*          Обработка исключений            */

    catch( const char *s ){
        log(s);
    }
    catch(...){
        log( "Неизвестная ошибка обработки" );
    }
}
