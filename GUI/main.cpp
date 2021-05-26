#include "GUI.h"
#include <qfile.h>
#include <qdebug.h>
#include <qdebug.h>

#include "CriminisiJob.h"
#include "Config.h"

#include <iostream>
#include <vector>

using namespace std;

Config config;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GUI w;
    CriminisiJob cjob;

    app.connect(&w, &GUI::sendfilename, &config, &Config::recvivefilename);
    app.connect(&w, &GUI::sendmaskfilename, &config, &Config::recvivemaskfilename);

    app.connect(&config, &Config::filenameisready, &cjob, &CriminisiJob::receiveImagefilename);
    app.connect(&config, &Config::maskfilenameisready, &cjob, &CriminisiJob::receiveMaskfilename);

    app.connect(&cjob, &CriminisiJob::srcImageisready, &w, &GUI::showMatImage);
    app.connect(&w, &GUI::maskImageIsReady, &cjob, &CriminisiJob::receiveMaskImage);

    w.show();
    app.exec();


    return 1;
}
