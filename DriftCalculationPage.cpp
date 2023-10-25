#include "qdebug.h"
#include "DriftCalculationPage.h"
#include "ui_DriftCalculationPage.h"

DriftCalculationPage::DriftCalculationPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DriftCalculationPage)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    setWindowTitle("漂移量计算");
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowMinMaxButtonsHint & ~Qt::WindowMaximizeButtonHint); // 去掉configui界面的最小化和最大化按钮

    QIntValidator *validator = new QIntValidator(0, 2048, this);
    ui->lineEdit_endPoint1->setValidator(validator);
    ui->lineEdit_endPoint2->setValidator(validator);
    ui->lineEdit_endPoint3->setValidator(validator);
    ui->lineEdit_endPoint4->setValidator(validator);

    ui->lineEdit_startPoint1->setValidator(validator);
    ui->lineEdit_startPoint2->setValidator(validator);
    ui->lineEdit_startPoint3->setValidator(validator);
    ui->lineEdit_startPoint4->setValidator(validator);

    ui->pushButton_StartCalcul->setText("开始计算");
    ui->pushButton_Cancel->setText("取消");

    ui->lineEdit_drift1->setReadOnly(true);
    ui->lineEdit_drift2->setReadOnly(true);
    ui->lineEdit_drift3->setReadOnly(true);
    ui->lineEdit_drift4->setReadOnly(true);

    ui->lineEdit_startPoint1->setText(QString::number(620));
    ui->lineEdit_startPoint2->setText(QString::number(932));
    ui->lineEdit_startPoint3->setText(QString::number(1430));
    ui->lineEdit_startPoint4->setText(QString::number(1610));

    ui->lineEdit_endPoint1->setText(QString::number(635));
    ui->lineEdit_endPoint2->setText(QString::number(936));
    ui->lineEdit_endPoint3->setText(QString::number(1434));
    ui->lineEdit_endPoint4->setText(QString::number(1614));

    QObject::connect(ui->pushButton_StartCalcul, &QPushButton::clicked, this, &DriftCalculationPage::slotStartResolutionCalculation);
    QObject::connect(ui->pushButton_Cancel, &QPushButton::clicked, this, [this]() {
        this->close();
    });
}

DriftCalculationPage::~DriftCalculationPage()
{
    delete ui;
}

int DriftCalculationPage::getValueFromLineEdit(QLineEdit *lineEdit)
{
    QString text = lineEdit->text();
    bool ok;
    int value = text.toInt(&ok);
    if (ok) {
        return value;
    } else {
        return -1;
    }
}

double DriftCalculationPage::corrcoef(int n, const double *a, const double *b)
{
    if (n <= 0 || a == nullptr || b == nullptr) {
        return 0.0;
    }

    double amean = std::accumulate(a, a + n, 0.0) / n;
    double bmean = std::accumulate(b, b + n, 0.0) / n;

    double sa = 0.0;
    double sb = 0.0;
    double cov = 0.0;

    for (int i = 0; i < n; ++i) {
        sa  += (a[i] - amean) * (a[i] - amean);
        sb  += (b[i] - bmean) * (b[i] - bmean);
        cov += (a[i] - amean) * (b[i] - bmean);
    }

    sa = sqrt(sa);
    sb = sqrt(sb);

    if (sa * sb > 1e-6) {
        return cov / (sa * sb);
    } else {
        return 0.0;
    }
}

/*************************************************
 * 函数类别：私有成员函数
 * 函数功能：追赶法求解带状线性方程组
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/14
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void DriftCalculationPage::chase(double *a, double *b, double *c, double *d, double *s, int n)
{
    if (!a || !b || !c || !d || !s || n <= 0) {
        return;
    }

    QVector<double> r(n + 2, 0.0);
    QVector<double> x(n + 2, 0.0);
    QVector<double> y(n + 2, 0.0);

    r[0] = c[0] / b[0];
    y[0] = d[0] / b[0];

    for (int i = 1; i < n; ++i) {
        r[i] = c[i] / (b[i] - a[i] * r[i - 1]);
        y[i] = (d[i] - a[i] * y[i - 1]) / (b[i] - a[i] * r[i - 1]);
    }

    x[n - 1] = y[n - 1];

    for (int i = n - 2; i >= 0; --i) {
        x[i] = y[i] - r[i] * x[i + 1];
    }

    for (int i = 0; i < n; ++i) {
        s[i + 1] = static_cast<double>(x[i]);
    }

    s[0] = 0.0;
    s[n + 1] = 0.0;
}

void DriftCalculationPage::spline(int n, double* spec_in, int m, double* spec_out, int n_interp)
{
    Q_UNUSED(n_interp)

    std::vector<double> a(n + 6);
    std::vector<double> b(n + 6);
    std::vector<double> c(n + 6);
    std::vector<double> d(n + 6);
    std::vector<double> s(n + 6);
    std::vector<double> x(n + 6);

    std::vector<double> a1(n + 6);
    std::vector<double> b1(n + 6);
    std::vector<double> c1(n + 6);
    std::vector<double> d1(n + 6);

    int i, j, k;
    double step, h;

    const double* y = spec_in;
    double* yy = spec_out;

    if (!y || !yy) {
        return;
    }

    for (i = 0; i < n; ++i) {
        x[i] = i;
    }

    step = (x[n - 1] - x[0]) / (m - 1.0);
    newspec[0] = x[0];
    newspec[m - 1] = x[n - 1];

    for (i = 1; i < m - 1; ++i) {
        double tmp = newspec[i - 1] + step;
        if (tmp < x[n - 1]) {
            newspec[i] = tmp;
        }
    }

    a[0] = 0;
    for (i = 1; i < n - 2; ++i) {
        a[i] = x[i] - x[i - 1];
    }

    for (i = 0; i < n - 2; ++i) {
        b[i] = 2 * (x[i + 1] - x[i] + x[i + 2] - x[i + 1]);
    }

    for (i = 0; i < n - 3; ++i) {
        c[i] = x[i + 2] - x[i + 1];
    }
    c[n - 3] = 0;

    for (i = 0; i < n - 2; ++i) {
        d[i] = 6 * (y[i + 2] - y[i + 1]) / (x[i + 2] - x[i + 1]) - 6 * (y[i + 1] - y[i]) / (x[i + 1] - x[i]);
    }

    chase(&a[0], &b[0], &c[0], &d[0], &s[0], n - 2);

    for (i = 0; i < n - 1; ++i) {
        a1[i] = (s[i + 1] - s[i]) / 6 / (x[i + 1] - x[i]);
        b1[i] = s[i] / 2;
        c1[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]) - (2 * (x[i + 1] - x[i]) * s[i] + (x[i + 1] - x[i]) * s[i + 1]) / 6;
        d1[i] = y[i];
    }

    i = 0;
    k = 0;

    for (j = 0; j < m; ++j) {
        if (newspec[j] > x[i]) {
            k = i;
            ++i;
        }

        h = newspec[j] - x[k];
        yy[j] = a1[k] * pow(h, 3) + b1[k] * pow(h, 2) + c1[k] * h + d1[k];
    }
}


double DriftCalculationPage::Cal_Wave_Shift2(double *spectra, int first_pixel, int last_pixel, double *refer_spec)
{
    if (spectra == nullptr || refer_spec == nullptr) {
        return -1.0;
    }

    int n_refer_spec = last_pixel - first_pixel + 1;

    const int nTmp = 11;
    double newspec2[1926];
    double refer_interp[1926];
    spline(n_refer_spec + nTmp * 2, &spectra[first_pixel - nTmp], 5 * (n_refer_spec + nTmp * 2 - 1) + 1, newspec2, 5);
    spline(n_refer_spec, refer_spec, 5 * (n_refer_spec - 1) + 1, refer_interp, 5);

    const int i_start = 5 * nTmp;
    const int i_end = 5 * nTmp + 5 * (n_refer_spec - 1);
    int n_shift;
    Calibration_Wave1(5 * (n_refer_spec + nTmp * 2 - 1) + 1, newspec2, i_start, i_end, i_end - i_start + 1, refer_interp, 5 * (nTmp-1), &n_shift);

    return 0.2 * static_cast<double>(n_shift);
}

int DriftCalculationPage::Calibration_Wave1(int nLength, double *spectra, int first_pixel, int last_pixel, int n_refer_spec, const double *refer_spec, int maxPixelShift, int *nShiftPixel)
{
    if (spectra == nullptr || refer_spec == nullptr)
    {
        return -1;
    }
    if (n_refer_spec != last_pixel - first_pixel + 1)
    {
        return -1;
    }

    const int idx_n = last_pixel - first_pixel + 1;
    const int nShift = maxPixelShift;
    double newspec1[2048];
    std::copy(spectra, spectra + nLength, newspec1);

    double coef_max = 0.0;
    int imax = 0;

    for (int i = -nShift; i < nShift + 1; ++i)
    {
        const double temp = corrcoef(idx_n, &newspec1[first_pixel + i], refer_spec);
//        qDebug() << "i" << i << "temp" << temp;
        if (temp > coef_max)
        {
            coef_max = temp;
            imax = i;
        }
    }

    if (coef_max > 0.95)
    {
//        qDebug() << "real imax" << imax;
        if (imax >= 0)
        {
            std::copy(spectra + imax, spectra + nLength, newspec1);
            std::fill(&newspec1[nLength - imax], &newspec1[nLength], 0.0);
        }
        else
        {
            std::fill(&newspec1[0], &newspec1[-imax], 0.0);
            std::copy(spectra, spectra + nLength + imax, &newspec1[-imax]);
        }

        std::copy(newspec1, newspec1 + nLength, spectra);

        if (nShiftPixel != nullptr)
        {
            *nShiftPixel = -imax;
        }
        return 1;
    }

    if (nShiftPixel != nullptr)
    {
        *nShiftPixel = 0.0;
    }
    return 0;
}

double DriftCalculationPage::newCalWaveShift2(double *spectra, int first_pixel, int last_pixel, double *refer_spec)
{
    if (!spectra || !refer_spec)
        return -1.0;

    int n_refer_spec = last_pixel - first_pixel + 1;
    int nTmp = 11;
    int n_total = n_refer_spec + nTmp * 2;
    int n_interp = 5 * (n_total - 1) + 1;
    int i_start = 5 * nTmp;
    int i_end = i_start + 5 * (n_refer_spec - 1);

    double* newspec2 = new double[n_interp];
    double* refer_interp = new double[n_interp];

    spline(n_total, spectra + first_pixel - nTmp, n_interp, newspec2, 5);
    spline(n_refer_spec, refer_spec, n_interp, refer_interp, 5);

    int n_shift;
    Calibration_Wave1(n_interp, newspec2, i_start, i_end, i_end - i_start + 1, refer_interp, 5 * (nTmp - 1), &n_shift);

    delete[] newspec2;
    delete[] refer_interp;

    return 0.2 * static_cast<double>(n_shift);
}


/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：串口发送新数据时，将新数据保存到本地变量
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/14
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void DriftCalculationPage::slotNewData(QVector<double> data)
{
    if (isCalculating && isReferenceSpectrumImported) {
        for (int i = 0; i < 2048; i++) {
            newspec[i] = data.at(i);
        }

#if 1
        double offset1 = Cal_Wave_Shift2/*newCalWaveShift2*/(newspec, ui->lineEdit_startPoint1->text().toInt(), ui->lineEdit_endPoint1->text().toInt(), &refspec[ui->lineEdit_startPoint1->text().toInt()]);
        double offset2 = Cal_Wave_Shift2/*newCalWaveShift2*/(newspec, ui->lineEdit_startPoint2->text().toInt(), ui->lineEdit_endPoint2->text().toInt(), &refspec[ui->lineEdit_startPoint2->text().toInt()]);
        double offset3 = Cal_Wave_Shift2/*newCalWaveShift2*/(newspec, ui->lineEdit_startPoint3->text().toInt(), ui->lineEdit_endPoint3->text().toInt(), &refspec[ui->lineEdit_startPoint3->text().toInt()]);
        double offset4 = Cal_Wave_Shift2/*newCalWaveShift2*/(newspec, ui->lineEdit_startPoint4->text().toInt(), ui->lineEdit_endPoint4->text().toInt(), &refspec[ui->lineEdit_startPoint4->text().toInt()]);
#else
        double offset1 = /*Cal_Wave_Shift2*/newCalWaveShift2(newspec, ui->lineEdit_startPoint1->text().toInt(), ui->lineEdit_endPoint1->text().toInt(), &refspec[ui->lineEdit_startPoint1->text().toInt()]);
        double offset2 = /*Cal_Wave_Shift2*/newCalWaveShift2(newspec, ui->lineEdit_startPoint2->text().toInt(), ui->lineEdit_endPoint2->text().toInt(), &refspec[ui->lineEdit_startPoint2->text().toInt()]);
        double offset3 = /*Cal_Wave_Shift2*/newCalWaveShift2(newspec, ui->lineEdit_startPoint3->text().toInt(), ui->lineEdit_endPoint3->text().toInt(), &refspec[ui->lineEdit_startPoint3->text().toInt()]);
        double offset4 = /*Cal_Wave_Shift2*/newCalWaveShift2(newspec, ui->lineEdit_startPoint4->text().toInt(), ui->lineEdit_endPoint4->text().toInt(), &refspec[ui->lineEdit_startPoint4->text().toInt()]);
#endif

        ui->lineEdit_drift1->setText(QString::number(offset1, 'f', 2));
        ui->lineEdit_drift2->setText(QString::number(offset2, 'f', 2));
        ui->lineEdit_drift3->setText(QString::number(offset3, 'f', 2));
        ui->lineEdit_drift4->setText(QString::number(offset4, 'f', 2));
    }
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：导入参考光谱时；
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/14
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void DriftCalculationPage::slotIsReferenceSpectrumImported(QVector<double> *data)
{
    for (int i = 0; i < 2048; i++) {
        refspec[i] = data->at(i);
    }
    isReferenceSpectrumImported = true;
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/14
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void DriftCalculationPage::slotStartResolutionCalculation()
{
    if (!isReferenceSpectrumImported) {
        QMessageBox::information(this, "LYDOAS", "请先导入参考光谱!");
        return;
    }

    if (ui->pushButton_StartCalcul->text() == "开始计算") {

        // 加一个判断，判断输入的起始点和结束点是否为空 20230810
        if (getValueFromLineEdit(ui->lineEdit_startPoint1) == -1 ||
            getValueFromLineEdit(ui->lineEdit_startPoint2) == -1 ||
            getValueFromLineEdit(ui->lineEdit_startPoint3) == -1 ||
            getValueFromLineEdit(ui->lineEdit_startPoint4) == -1 ||
            getValueFromLineEdit(ui->lineEdit_endPoint1)   == -1 ||
            getValueFromLineEdit(ui->lineEdit_endPoint2)   == -1 ||
            getValueFromLineEdit(ui->lineEdit_endPoint3)   == -1 ||
            getValueFromLineEdit(ui->lineEdit_endPoint4)   == -1
            ) {
            QMessageBox::information(this, "LYDOAS", "请输入完整的起始点和结束点！");
            return;
        }
        if (getValueFromLineEdit(ui->lineEdit_endPoint1) - getValueFromLineEdit(ui->lineEdit_startPoint1) < 2 ||
            getValueFromLineEdit(ui->lineEdit_endPoint2) - getValueFromLineEdit(ui->lineEdit_startPoint2) < 2 ||
            getValueFromLineEdit(ui->lineEdit_endPoint3) - getValueFromLineEdit(ui->lineEdit_startPoint3) < 2 ||
            getValueFromLineEdit(ui->lineEdit_endPoint4) - getValueFromLineEdit(ui->lineEdit_startPoint4) < 2
            ) {
            QMessageBox::information(this, "LYDOAS", "请输入合理的起始点和结束点！(结束点至少高于起始点2)");
            return;
        }

        if (getValueFromLineEdit(ui->lineEdit_endPoint1) - getValueFromLineEdit(ui->lineEdit_startPoint1) > 40 ||
            getValueFromLineEdit(ui->lineEdit_endPoint2) - getValueFromLineEdit(ui->lineEdit_startPoint2) > 40 ||
            getValueFromLineEdit(ui->lineEdit_endPoint3) - getValueFromLineEdit(ui->lineEdit_startPoint3) > 40 ||
            getValueFromLineEdit(ui->lineEdit_endPoint4) - getValueFromLineEdit(ui->lineEdit_startPoint4) > 40
            ) {
            QMessageBox::information(this, "LYDOAS", "请输入合理的起始点和结束点！(结束点至起始点范围最大40)");
            return;
        }

        ui->pushButton_StartCalcul->setText("停止计算");
        isCalculating = true;

        ui->lineEdit_startPoint1->setReadOnly(true);
        ui->lineEdit_startPoint2->setReadOnly(true);
        ui->lineEdit_startPoint3->setReadOnly(true);
        ui->lineEdit_startPoint4->setReadOnly(true);

        ui->lineEdit_endPoint1->setReadOnly(true);
        ui->lineEdit_endPoint2->setReadOnly(true);
        ui->lineEdit_endPoint3->setReadOnly(true);
        ui->lineEdit_endPoint4->setReadOnly(true);
    } else {
        ui->pushButton_StartCalcul->setText("开始计算");
        isCalculating = false;

        ui->lineEdit_startPoint1->setReadOnly(false);
        ui->lineEdit_startPoint2->setReadOnly(false);
        ui->lineEdit_startPoint3->setReadOnly(false);
        ui->lineEdit_startPoint4->setReadOnly(false);

        ui->lineEdit_endPoint1->setReadOnly(false);
        ui->lineEdit_endPoint2->setReadOnly(false);
        ui->lineEdit_endPoint3->setReadOnly(false);
        ui->lineEdit_endPoint4->setReadOnly(false);
    }
}

