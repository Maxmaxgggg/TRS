#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "RealFIR.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE


struct Result {
    QString textASCII;
    int nonASCII;
};

struct Symbol {
    int     size;
    float   *data;
    Symbol( int s ) {
        size    = s;
        data    = new float[size];
    }
    ~Symbol() {
        delete[] data;
    }
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
private:

    const int       SMPL_FRQ    = 44100; // Частота дискретизации
    const int       CARRIER_FRQ = 11025; // Частота несущего колебания
    const float     CARRIER_AMP = 3000;
    const int       S_SIZE      = 1102;  // Floor(CARRIER_FRQ*PACK_DUR)
    const float     PACK_DUR    = 0.025;
    const int       BUF_SIZE    = 1024*1024; //выходные значения(набор амплитуд) записались в массив такого размера

private slots:

    void fileSelectSlot();

    void on_progressPBN_clicked();

private:

    Ui::Widget              *ui;

    QHash<int,Symbol*>      alphabet;

    float                   *buf;

    RealFIR                 *filter;



    void                    clearAlphabet();

    void                    log( const QString &s );

    void                    addSymbol( int c );

    Result                  rmNonASCII( const QString& input );

    QVector<int>            cvrtToBit( const QString& input );
};
#endif // WIDGET_H
