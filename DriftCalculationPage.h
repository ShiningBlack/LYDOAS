#ifndef DRIFTCALCULATIONPAGE_H
#define DRIFTCALCULATIONPAGE_H

#include "qlineedit.h"
#include <QWidget>
#include <QIntValidator>
#include <QMessageBox>
#include <QDebug>
#include <QList>


#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

namespace Ui {
class DriftCalculationPage;
}

constexpr int kMaxSpecLen = 10000;
constexpr int kMaxSplineOrder = 4;
constexpr int kMaxChaseOrder = 3;
constexpr double kCorrThreshold = 0.95;

class DriftCalculationPage : public QWidget
{
    Q_OBJECT
    friend class MainWindow;
public:
    explicit DriftCalculationPage(QWidget *parent = nullptr);
    ~DriftCalculationPage();

protected:

private:
    Ui::DriftCalculationPage *ui;
    double newspec[2048] = {0};
    double refspec[2048] = {0};

    bool isCalculating = false;
    bool isReferenceSpectrumImported = false;

    int getValueFromLineEdit(QLineEdit* lineEdit);

    double corrcoef(int n,const double *a,const double *b);
    void chase(double *a,double *b,double *c,double *d,double *s,int n);
    void spline(int n,double *spec_in,int m,double *spec_out,int n_interp);
    double Cal_Wave_Shift2(double *spectra, int first_pixel, int last_pixel, double *refer_spec/*, double *nShiftPixel*/);
    int Calibration_Wave1(int nLength, double *spectra, int first_pixel, int last_pixel, int n_refer_spec, const double *refer_spec, int maxPixelShift, int *nShiftPixel);

    double newCalWaveShift2(double *spectra, int first_pixel, int last_pixel, double *refer_spec);

public slots:
    void slotNewData(QVector<double> data);
    void slotIsReferenceSpectrumImported(QVector<double> *);
    void slotStartResolutionCalculation();
};

#endif // DRIFTCALCULATIONPAGE_H
