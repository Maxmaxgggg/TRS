﻿//---------------------------------------------------------------------------
#ifndef RealFIRH
#define RealFIRH
//---------------------------------------------------------------------------
#include <QString>
#include <QList>
//---------------------------------------------------------------------------
// ВЕРСИЯ 1.30
//---------------------------------------------------------------------------
class RealFIR {
public:
 // тип фильтра
    typedef enum {
        Lowpass,    // фильтр нижних частот
        Highpass,   // фильтр верхних частот
        Bandpass,   // полосовой фильтр
        Bandstop    // режекторный фильтр
    } FltrType;
 // тип оконной функции
    typedef enum {
        WinRectangular, // прямоугольное окно
        WinChebyshev    // окно Чебышева
    } FltrWinType;
public:
    typedef enum {
    // фильтрация начинается с линии задержки,
    // полностью заполненной входными данными,
    // на выходе получаем ДЛИНА_РЕАЛИЗАЦИИ - ДЛИНА_ФИЛЬТРА + 1
    // ПЕРЕХОДНЫЙ ПРОЦЕСС ОТСУТСТВУЕТ
                FullDlyLine = 1,
    // фильтрация начинается с линни задержки,
    // заполненной входными данными на половину
    // на выходе ПОЛУЧАЕМ СТОЛЬКО ЖЕ, СКОЛЬКО И ПЕРЕДАЛИ
    // ПЕРЕХОДНЫЙ ПРОЦЕСС МИНИМИЗИРОВАН
    // !!! ПАРАМЕТР ПО УМОЛЧАНИЮ !!!B
                HalfDlyLine = 2,
    // фильтрация начинается с линии задержки,
    // заполненной нулями
    // на выходе ПОЛУЧАЕМ СТОЛЬКО ЖЕ, СКОЛЬКО И ПЕРЕДАЛИ
    // переходный процесс в начале - полностью, в конце реализации - отсутствует
                EmptyDlyLine = 3
            } Borders;
public:
    // в конструкоте задаются параметры фильтра.
    // для базового использования надо задать:
    // тип фильтра     -  ФНЧ/ФВЧ/полосовой/режекторный
    // первая частота  -  частота среза для ФНЧ/ФВЧ, одна из двух частот для полосового/режекторного
    // вторая частота  -  требуется для полосового/режекторного фильтра
            RealFIR( FltrType       type,   // тип фильтра, константы см. выше
                     double         freq1,  // первая характерная частота - имеет смысл для всех фильтров
                     double         freq2,  // вторая характерная частота - имеет смысл только для полосовых и режекторных
                     double         sfrq        = 44100,         // частота дискретизации
                     FltrWinType    winType     = WinChebyshev,  // тип окна для рассчета фильтра
                     double         winLvl      = 80,            // уровень подавления
                     int            tCount      = 257,           // число коэффициентов фильтра
                     Borders        strategy    = EmptyDlyLine );// стратегия обработки начала и конца реализации
           ~RealFIR( void );

    // функция сбрасывает фильтр в начальное состояние, по умолчанию число пространственных каналов НЕ ИЗМЕНЯЕТСЯ
    void    reset();

    // функция фильтрации
    //  - src может совпадать с dst, но !!!ОБЪЕМ ДАННЫХ НА ВЫХОДЕ МОЖЕТ БЫТЬ БОЛЬШЕ ОБЪЕМА ДАННЫХ НА ВХОДЕ!!!
    //  - при фильтрации первого блока размер на выходе может быть меньше переданного !
    // особо следует пояснить поведение при finalize = true:
    //  - в зависимости от выбранной стратегии обработки !!!РАЗМЕР НА ВЫХОДЕ МОЖЕТ БЫТЬ БОЛЬШЕ ВХОДНОГО!!!
    //  - !!!ФИЛЬТР НЕ СБРАСЫВАЕТСЯ!!!
    //  - !!!ДАЛЬНЕЙШАЯ ФИЛЬТРАЦИЯ НЕ ПРОИЗВОДИТСЯ!!! т.е. все последующие вызовы игнорируются - надо вызвать Reset
    //  - может вызываться сразу после сброса - т.е. одним вызовом фильтрация целого блока с учетом левой и правой границ
    void    filter(const float    *src,                 // исходная реализация
                    int            srcSize,             // число ОТСЧЕТОВ на входе
                    float          *dst,                // массив для сохранения результатов фильтрации
                    int            *dstSize,            // число ОТСЧЕТОВ на выходе фильтра
                    bool           fin = false );       // флаг должен быть true для фильтрации последнего блока данных !!!РАЗМЕР НА ВЫХОДЕ МОЖЕТ БЫТЬ БОЛЬШЕ ВХОДНОГО!!!
    // тоже самое, но с целочисленным типом
    void    filter( const qint16   *src,
                    int            srcSize,
                    qint16         *dst,
                    int            *dstSize,
                    bool           fin = false );

    // методы для получения параметров фильтра
    double          getSmplFreq()  { return smplFreq; }
    int             getTapsCount() { return tapCnt; }
    QList<float>    getTaps()
                    {
                        QList<float> result;

                        for( int i = 0 ; i < tapCnt ; ++i ) {
                            result.append( taps[i] );
                        }
                        return result;
                    }
private:
    bool                error;

    int                 blockSizeSMPL;  // число ОТСЧЕТОВ, которым ведется обработка
    int                 tapCnt;         // длина линии задержки фильтра

    bool                finalized;      // true если произошла финализация

    Borders             bStrat;         // политика обработки границ фильтруемых данных, по умолчанию равна HalfDlyLine
    int                 dlyLineBefore;  // сколько ВХОДНЫХ отсчетов надо передать в фильтр, пержде чем начать формировать выходную последовательность
    int                 dlyLineAfter;   // сколько НУЛЕВЫХ отсчетов надо передать в фильтр при завершении фильтрации

    double              smplFreq;

    float               *taps;          // коэффициенты фильтра
    float               *firDly;        // линия задержки для работы фильтра
    float               *tmpF;          // временный массив
    int                 firDlyPtr;      // указатель в линии задержки

    const int           BLOCK_SIZE = 1024*10;

    const int           BUF16 = 1024;
    float               *src16Buf;      // буфер для работы с short
    float               *dst16Buf;      // буфер для работы с short

    void    genFilter( double        freq1,
                       double        freq2,
                       FltrType      fltType,
                       float         *t,
                       int           tCnt,
                       FltrWinType   winType,
                       double        winLvl );

    void    setBSValues( void );    // функция устанавливает счетчики переходных процессов в соответствии с выбранной стратегией

    void    initMemory(int              tCnt );            // функция инициализации переменных и выделения памяти под массивы
    
    void    internalFIR( const float    *src,
                         int            srcSize,
                         float          *dst );

    void    outputErr( const QString &problem,
                       const QString &solve );

    // функция расчета окна Чебышева с произвольным подавлением
    void    chebyshev_window(float beta, float *win_chReal, int sizeFFT );
    void    realInvDft( float *real_mas, int razmFFT, float *result );

    void    setBorderStrategy( Borders strategy );
    Borders getBorderStrategy( void );

    double  sinc(double x );
};
//---------------------------------------------------------------------------
#endif
