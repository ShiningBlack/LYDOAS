#ifndef RESOLUTIONCALCULATIONPAGE_H
#define RESOLUTIONCALCULATIONPAGE_H

#include <QWidget>
#include <QPalette>
#include <QByteArray>
#include <QIntValidator>
#include <QMessageBox>
#include <cmath>

namespace Ui {
class ResolutionCalculationPage;
}

class ResolutionCalculationPage : public QWidget
{
    Q_OBJECT
    friend class MainWindow;
public:
    explicit ResolutionCalculationPage(QWidget *parent = nullptr);
    ~ResolutionCalculationPage();

    typedef struct
    {
        float wave;
        quint16 i_start;
        quint16 i_end;
    }stru_peak;

    // 计算光谱分辨率相关
    stru_peak peak1 {253.0, 350, 450};
    stru_peak peak2 {436.0, 1400, 1500};
    stru_peak peak3 {0.0, 1200, 1300};

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::ResolutionCalculationPage *ui;
    QVector<double> seriesData;

    bool isCalculating = false; // 实时计算标志位

    // 计算光谱分辨率相关
    double Func_Cal_Wave_Resolution(quint16 nLength, double *spectrum,stru_peak peak1,stru_peak peak2,int n_interp);
    double Func_Cal_semiband_width(unsigned short nLength, double *spectrum,stru_peak peak,int n_interp);
    void Func_max_interp(double *arr, quint16 arr_n, quint16 *index_max, quint16 n_interp);
    void Func_max(double *arr,unsigned short arr_n,unsigned short *index_max);
    void spline(int n,double *spec_in,int m,double *spec_out,int n_interp);
    void chase(double *a,double *b,double *c,double *d,double *s,int n);

private slots:
    void slotStartResolutionCalculation();
    void slotGetCurrentSerialData(QVector<double> data);

    void slotNewData(QVector<double> data);
};

#endif // RESOLUTIONCALCULATIONPAGE_H
