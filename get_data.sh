#!/bin/bash

curl http://picayune.uclick.com/comics/tmcal/data/tmcal$(date +%y%m%d)-data.xml -o latimes.xml
curl http://picayune.uclick.com/comics/usaon/data/usaon$(date +%y%m%d)-data.xml -o usatoday.xml
curl http://picayune.uclick.com/comics/fcx/data/fcx$(date +%y%m%d)-data.xml -o fcx.xml
