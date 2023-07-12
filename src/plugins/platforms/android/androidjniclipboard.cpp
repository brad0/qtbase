/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "androidjniclipboard.h"
#include <QtCore/QUrl>
#include <QtCore/QJniObject>
#include <QtCore/QJniEnvironment>

QT_BEGIN_NAMESPACE

using namespace QtAndroid;
namespace QtAndroidClipboard
{
    QAndroidPlatformClipboard *m_manager = nullptr;

    static JNINativeMethod methods[] = {
        {"onClipboardDataChanged", "()V", (void *)onClipboardDataChanged}
    };

    void setClipboardManager(QAndroidPlatformClipboard *manager)
    {
        m_manager = manager;
        QJniObject::callStaticMethod<void>(applicationClass(), "registerClipboardManager");
        jclass appClass = QtAndroid::applicationClass();
        QJniEnvironment env;
        if (env->RegisterNatives(appClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
            __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
            return;
        }
    }
    void clearClipboardData()
    {
        QJniObject::callStaticMethod<void>(applicationClass(), "clearClipData");
    }
    void setClipboardMimeData(QMimeData *data)
    {
        clearClipboardData();
        if (data->hasUrls()) {
            QList<QUrl> urls = data->urls();
            for (const auto &u : qAsConst(urls)) {
                QJniObject::callStaticMethod<void>(applicationClass(),
                                                   "setClipboardUri",
                                                   "(Ljava/lang/String;)V",
                                                   QJniObject::fromString(u.toEncoded()).object());
            }
        } else if (data->hasHtml()) { // html can contain text
            QJniObject::callStaticMethod<void>(applicationClass(),
                                               "setClipboardHtml",
                                               "(Ljava/lang/String;Ljava/lang/String;)V",
                                               QJniObject::fromString(data->text()).object(),
                                               QJniObject::fromString(data->html()).object());
        } else if (data->hasText()) { // hasText must be the last (the order matter here)
            QJniObject::callStaticMethod<void>(applicationClass(),
                                               "setClipboardText", "(Ljava/lang/String;)V",
                                               QJniObject::fromString(data->text()).object());
        }
    }

    QMimeData *getClipboardMimeData()
    {
        QMimeData *data = new QMimeData;
        if (QJniObject::callStaticMethod<jboolean>(applicationClass(), "hasClipboardText")) {
            data->setText(QJniObject::callStaticObjectMethod(applicationClass(),
                                                             "getClipboardText",
                                                             "()Ljava/lang/String;").toString());
        }
        if (QJniObject::callStaticMethod<jboolean>(applicationClass(), "hasClipboardHtml")) {
            data->setHtml(QJniObject::callStaticObjectMethod(applicationClass(),
                                                             "getClipboardHtml",
                                                             "()Ljava/lang/String;").toString());
        }
        if (QJniObject::callStaticMethod<jboolean>(applicationClass(), "hasClipboardUri")) {
            QJniObject uris = QJniObject::callStaticObjectMethod(applicationClass(),
                                                                 "getClipboardUris",
                                                                 "()[Ljava/lang/String;");
            if (uris.isValid()) {
                QList<QUrl> urls;
                QJniEnvironment env;
                jobjectArray juris = uris.object<jobjectArray>();
                const jint nUris = env->GetArrayLength(juris);
                urls.reserve(static_cast<int>(nUris));
                for (int i = 0; i < nUris; ++i)
                    urls << QUrl(QJniObject(env->GetObjectArrayElement(juris, i)).toString());
                data->setUrls(urls);
            }
        }
        return data;
    }

    void onClipboardDataChanged(JNIEnv */*env*/, jobject /*thiz*/)
    {
        m_manager->emitChanged(QClipboard::Clipboard);
    }
}

QT_END_NAMESPACE
