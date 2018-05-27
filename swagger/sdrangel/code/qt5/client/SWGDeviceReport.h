/**
 * SDRangel
 * This is the web REST/JSON API of SDRangel SDR software. SDRangel is an Open Source Qt5/OpenGL 3.0+ (4.3+ in Windows) GUI and server Software Defined Radio and signal analyzer in software. It supports Airspy, BladeRF, HackRF, LimeSDR, PlutoSDR, RTL-SDR, SDRplay RSP1 and FunCube     ---   Limitations and specifcities:       * In SDRangel GUI the first Rx device set cannot be deleted. Conversely the server starts with no device sets and its number of device sets can be reduced to zero by as many calls as necessary to /sdrangel/deviceset with DELETE method.   * Stopping instance i.e. /sdrangel with DELETE method is a server only feature. It allows stopping the instance nicely.   * Preset import and export from/to file is a server only feature.   * Device set focus is a GUI only feature.   * The following channels are not implemented (status 501 is returned): ATV demodulator, Channel Analyzer, Channel Analyzer NG, LoRa demodulator, TCP source   * The content type returned is always application/json except in the following cases:     * An incorrect URL was specified: this document is returned as text/html with a status 400    --- 
 *
 * OpenAPI spec version: 4.0.0
 * Contact: f4exb06@gmail.com
 *
 * NOTE: This class is auto generated by the swagger code generator program.
 * https://github.com/swagger-api/swagger-codegen.git
 * Do not edit the class manually.
 */

/*
 * SWGDeviceReport.h
 *
 * Base device report. The specific device report present depends on deviceHwType
 */

#ifndef SWGDeviceReport_H_
#define SWGDeviceReport_H_

#include <QJsonObject>


#include "SWGAirspyHFReport.h"
#include "SWGAirspyReport.h"
#include "SWGFileSourceReport.h"
#include "SWGLimeSdrInputReport.h"
#include "SWGLimeSdrOutputReport.h"
#include "SWGPerseusReport.h"
#include "SWGPlutoSdrInputReport.h"
#include "SWGPlutoSdrOutputReport.h"
#include "SWGRtlSdrReport.h"
#include "SWGSDRPlayReport.h"
#include "SWGSDRdaemonSourceReport.h"
#include <QString>

#include "SWGObject.h"
#include "export.h"

namespace SWGSDRangel {

class SWG_API SWGDeviceReport: public SWGObject {
public:
    SWGDeviceReport();
    SWGDeviceReport(QString* json);
    virtual ~SWGDeviceReport();
    void init();
    void cleanup();

    virtual QString asJson () override;
    virtual QJsonObject* asJsonObject() override;
    virtual void fromJsonObject(QJsonObject &json) override;
    virtual SWGDeviceReport* fromJson(QString &jsonString) override;

    QString* getDeviceHwType();
    void setDeviceHwType(QString* device_hw_type);

    qint32 getTx();
    void setTx(qint32 tx);

    SWGAirspyReport* getAirspyReport();
    void setAirspyReport(SWGAirspyReport* airspy_report);

    SWGAirspyHFReport* getAirspyHfReport();
    void setAirspyHfReport(SWGAirspyHFReport* airspy_hf_report);

    SWGFileSourceReport* getFileSourceReport();
    void setFileSourceReport(SWGFileSourceReport* file_source_report);

    SWGLimeSdrInputReport* getLimeSdrInputReport();
    void setLimeSdrInputReport(SWGLimeSdrInputReport* lime_sdr_input_report);

    SWGLimeSdrOutputReport* getLimeSdrOutputReport();
    void setLimeSdrOutputReport(SWGLimeSdrOutputReport* lime_sdr_output_report);

    SWGPerseusReport* getPerseusReport();
    void setPerseusReport(SWGPerseusReport* perseus_report);

    SWGPlutoSdrInputReport* getPlutoSdrInputReport();
    void setPlutoSdrInputReport(SWGPlutoSdrInputReport* pluto_sdr_input_report);

    SWGPlutoSdrOutputReport* getPlutoSdrOutputReport();
    void setPlutoSdrOutputReport(SWGPlutoSdrOutputReport* pluto_sdr_output_report);

    SWGRtlSdrReport* getRtlSdrReport();
    void setRtlSdrReport(SWGRtlSdrReport* rtl_sdr_report);

    SWGSDRdaemonSourceReport* getSdrDaemonSourceReport();
    void setSdrDaemonSourceReport(SWGSDRdaemonSourceReport* sdr_daemon_source_report);

    SWGSDRPlayReport* getSdrPlayReport();
    void setSdrPlayReport(SWGSDRPlayReport* sdr_play_report);


    virtual bool isSet() override;

private:
    QString* device_hw_type;
    bool m_device_hw_type_isSet;

    qint32 tx;
    bool m_tx_isSet;

    SWGAirspyReport* airspy_report;
    bool m_airspy_report_isSet;

    SWGAirspyHFReport* airspy_hf_report;
    bool m_airspy_hf_report_isSet;

    SWGFileSourceReport* file_source_report;
    bool m_file_source_report_isSet;

    SWGLimeSdrInputReport* lime_sdr_input_report;
    bool m_lime_sdr_input_report_isSet;

    SWGLimeSdrOutputReport* lime_sdr_output_report;
    bool m_lime_sdr_output_report_isSet;

    SWGPerseusReport* perseus_report;
    bool m_perseus_report_isSet;

    SWGPlutoSdrInputReport* pluto_sdr_input_report;
    bool m_pluto_sdr_input_report_isSet;

    SWGPlutoSdrOutputReport* pluto_sdr_output_report;
    bool m_pluto_sdr_output_report_isSet;

    SWGRtlSdrReport* rtl_sdr_report;
    bool m_rtl_sdr_report_isSet;

    SWGSDRdaemonSourceReport* sdr_daemon_source_report;
    bool m_sdr_daemon_source_report_isSet;

    SWGSDRPlayReport* sdr_play_report;
    bool m_sdr_play_report_isSet;

};

}

#endif /* SWGDeviceReport_H_ */
