TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS +=  Ts3Client \
            AudioBot \


AudioBot.depends = Ts3Client
