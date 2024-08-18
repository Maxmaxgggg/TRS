#include "widget.h" // Подключение заголовочного файла класса Widget
#include "ui_widget.h" // Подключение заголовочного файла, сгенерированного Qt Designer
#include <QTime> // Подключение класса времени Qt
#include <QFileDialog> // Подключение класса диалогового окна для выбора файлов
#include <cmath> // Подключение математических функций из стандартной библиотеки C++
#define USE_MATH_DEFINES // Определение макроса для использования математических констант

// Конструктор класса Widget
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget) // Инициализация объекта интерфейса
{
    ui->setupUi(this);
    ui->in_FilePBN->setProperty( "fileRole", 100 ); // Установка свойства "fileRole" для кнопки выбора входного файла
    ui->outFilePBN->setProperty( "fileRole", 101 ); // Установка свойства "fileRole" для кнопки выбора выходного файла


    buf = new float[BUF_SIZE];// new - выделение памяти
    //Создание фильтра
    filter = new RealFIR( RealFIR::Bandpass,
                          CARRIER_FRQ - 1000,
                          CARRIER_FRQ + 1000 );

    log( "Приложение запущено "); // Запись в лог о запуске приложения
}

// Деструктор класса Widget
Widget::~Widget()
{
    clearAlphabet(); // Очистка алфавита
    //освобождаем память
    delete      ui; // Освобождение памяти, выделенной для интерфейса
    delete[]    buf; // Освобождение памяти, выделенной для буфера
    delete      filter;  // Освобождение памяти, выделенной для фильтра
}

                                                            /*      МЕТОДЫ       */


// Метод вывода на экран действий пользователя
void Widget::log(const QString &s)
{
    QString m;//QString  позаоляет складывать строки
    m = QTime::currentTime().toString(); // Получение текущего времени
    m += "    "; // Добавление пробела
    m += s;      // Добавление сообщения
    ui->logPTE->appendPlainText( m ); // Добавление строки в лог
    qDebug() << m; // вывод сообщения в консоль отладки
}

// Метод очистки строки от символов не в кодировке ASCII7
Result Widget::rmNonASCII(const QString &input)
{
    QString res; // Создание строки для результата
    // переменная для подсчета кол-ва символов не явля-ся символами ASCII7
    // исп-ся для обнуления счетчика
    int nonASCII = 0;
    // Удаляеем все символы из строки не в кодировке ASCII7
    for ( QChar ch : input ) { // перербор каждого символа в строке
        if ( ch.unicode() < 128 ) {
            res.append( ch ); // Добавление символа в результат, если он в кодировке ASCII7
        } else {
            nonASCII++; // Увеличение счетчика символов не в кодировке ASCII7
        }
    }
    // Возвращение результата и количества символов не в кодировке ASCII7
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
        asciiValue  = ch.unicode();//ASCII код для текущего символа
        bitPart     = QString("%1").arg(asciiValue, 7, 2, QChar('0'));//код в двоичную строку длины 7 бит, заполняя ведущие нули
        bitString.append(bitPart);//добавляем двоичную строку текущего символа к общей строке bitString
    }

    // Если число бит нечетное, то добавляем в конец '0'
    if( bitString.size() % 2 != 0 )
        bitString.append('0');


    // Разбиваем строку битов на группы по 2 бита и преобразуем каждую группу в число от 0 до 3
    for (int i = 0; i < bitString.size(); i += 2) {
        bitPair = bitString.mid(i, 2);// Извлечение пары битов
        value = bitPair.toInt(nullptr, 2);// выбранные 2 бита преоьразуем в 10-ое значение value
        result.append( value ); // Добавление значения в результат
    }

    return result;
}

// Метод добавления символов 3,2,1,0 в алфавит
void Widget::addSymbol( int c )
{
    if( alphabet.contains(c) ) {// проверка содержится ли символ в алфавите
        delete alphabet[c]; // Удаление символа, если он существует
        alphabet.remove(c); // Удаление символа из алфавита
    }
    //каждый символ состоит из 1102 отсчетов
    alphabet[c] = new Symbol( S_SIZE );// создаем новый символ с заданным размером данных
    float amp = CARRIER_AMP;// установка амплитуды в зависимости от от значения с
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
        //  если произошла ошибка в блоке try, записываем сообщение в лог
        log(s);
    }
    for ( int i = 0; i < S_SIZE; ++i )
        alphabet[c]->data[i] = amp*cos( 2*M_PI*CARRIER_FRQ/SMPL_FRQ*i );// Заполнение данных символа значениями косинуса

}

// Метод очистки алфавита
void Widget::clearAlphabet()
{
    //пробегаеися по всем символам в алфавите
    foreach( Symbol *s, alphabet){
        // удаляем символы из памяти
        delete s;
    }
    //очищаем контейнер алфавита
    alphabet.clear();
}


                                                            /*      СЛОТЫ       */

// Метод выбора файла
void Widget::fileSelectSlot()
{
    QObject     *obj = sender(); // Получение объекта, вызвавшего слот

    if( obj != NULL ) {//объект вызвал слот
        QLineEdit   *led = NULL; // Переменная для хранения QLineEdit
        QString     str;// строка для лога

        int         role = obj->property("fileRole").toInt(); // Получение значения свойства "fileRole"
        switch( role ){
        case 100 :
            led = ui->in_FileLED; // Установка QLineEdit для входного файла
            str = "исходный файл"; // Установка строки для лога
            break;
        case 101 :
            led = ui->outFileLED; // Установка QLineEdit для выходного файла
            str = "выходной файл"; // Установка строки для лога
            break;
        };
        if( led != NULL ) {// если удалось определить QLineEdit, то вызывается окно, чтобы выбрать файл
            log( "Выбираем " + str );
            QString fileName = QFileDialog::getOpenFileName(// Вызов диалогового окна для выбора файла
                this,
                "Выберите " + str,
                "D:/");
            if( fileName.isEmpty() ) { // Проверка, выбран ли файл
                log( "Пользователь отказался от выбора файла" );
            } else {
                led->setText( fileName );// Установка выбранного имени файла в QLineEdit
            }
        } else {
            log( "Вызывающий объект неизвестен" ); // Запись в лог об ошибке
        }
    }
}

// Метод обработки нажатия кнопки "Прогресс"
void Widget::on_progressPBN_clicked()
{
    try {

        log( "Обработка запущена" ); // Запись в лог о начале обработки

        // Добавляем уровни сигнала в алфавит
        // 3 <-> +3, 2 <-> +1, 1 <-> -1, 0 <-> -3
        clearAlphabet(); // Очистка алфавита
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
           text = ui->mesLED->text(); // Сохранение введенного текста в переменную
        // Если пользователь выбрал файл
        if( !ui->in_FileLED->text().isEmpty() ) {
            // Если пользователь и выбрал файл, и ввел сообщение, то приоритет отдается файлу
            if( !text.isEmpty() )
                log( "Пользователь и выбрал файл, и ввел сообщение."
                     "Приоритет отдается файлу" );

            QString     fileName = ui->in_FileLED->text(); // Получение имени файла
            QFile       inFile( fileName ); // Открытие файла
            if( inFile.open( QIODevice::ReadOnly ) ) {
                // Создаем объект QTextStream для чтения из файла
                QTextStream in(&inFile);

                // Читаем все содержимое файла в QString
                text = in.readAll();

                // Закрываем файл
                inFile.close();
            } else {
                throw( "Ошибка при открытии исходного файла" ); // Генерация исключения при ошибке открытия файла
            }
        }
        // Если ни файл, ни текст не были введены
        if( ui->mesLED->text().isEmpty() && ui->in_FileLED->text().isEmpty() )
            throw( "Ошибка, пользователь не выбрал файл и не ввел сообщение" ); // Генерация исключения

                /*          Получаем вектор амплитуд импульсов              */

        log( "Приступаем к обработке данных" );

        // Убираем все символы не в кодировке ASCII7
        res         = rmNonASCII( text );
        textASCII   = res.textASCII;
        nonASCII    = res.nonASCII;
        ampVec      = cvrtToBit( textASCII ); // Преобразование текста в вектор амплитуд

        //количество удаленных символов не в кодировке ASCII7
        QString symbols;
        QString detect = ( nonASCII - 1 ) % 10 ? " обнаружено ": " обнаружен ";
        QString skip = nonASCII - 1 ? " Они будут пропущены": " Он будет пропущен";

        switch( nonASCII % 10 ){
            case 1 : symbols = " символ ";   break;
            case 2 : symbols = " символа ";  break;
            case 3 : symbols = " символа ";  break;
            case 4 : symbols = " символа ";  break;
            default: symbols = " символов "; break;
        }
        if( nonASCII != 0 ) {
            log( "В сообщении/файле" + detect + QString::number( nonASCII ) + symbols +
                 "не в кодировке ASCII7." + skip );
        }


                /*          Записываем результат в выходной файл            */

        // Открытие выходного файла для записи
        QFile outFile ( ui->outFileLED->text());
        if( !outFile.open( QIODevice::WriteOnly ) ) {
            throw( "Ошибка при открытии выходного файла" );
        }

        outFile.resize(0); // Очистка содержимого выходного файла
        int         fltSize;

        // Применение фильтра к каждому значению в векторе амплитуд и запись результата в файл
        foreach ( int c, ampVec ) {
           filter->filter( alphabet[c]->data,
                           alphabet[c]->size,
                           buf,
                           &fltSize );
           outFile.write( (char*) buf, fltSize*sizeof(float) );
        }
        log( "Обработка завершена. Результат сохранен в выходной файл" );
    }

        /*          Обработка исключений            */

    catch( const char *s ){
        log(s);
    }
    catch(...){
        log( "Неизвестная ошибка обработки" );
    }
}
