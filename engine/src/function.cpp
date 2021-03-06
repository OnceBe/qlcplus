/*
  Q Light Controller
  function.cpp

  Copyright (c) Heikki Junnila

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  Version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details. The license is
  in the file "COPYING".

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <QString>
#include <QDebug>
#include <QtXml>

#include "qlcmacros.h"
#include "qlcfile.h"

#include "mastertimer.h"
#include "collection.h"
#include "rgbmatrix.h"
#include "function.h"
#include "chaser.h"
#include "script.h"
#include "audio.h"
#include "scene.h"
#include "show.h"
#include "efx.h"
#include "doc.h"

const QString KSceneString      (      "Scene" );
const QString KChaserString     (     "Chaser" );
const QString KEFXString        (        "EFX" );
const QString KCollectionString ( "Collection" );
const QString KScriptString     (     "Script" );
const QString KRGBMatrixString  (  "RGBMatrix" );
const QString KShowString       (       "Show" );
const QString KAudioString      (      "Audio" );
const QString KUndefinedString  (  "Undefined" );

const QString KLoopString       (       "Loop" );
const QString KPingPongString   (   "PingPong" );
const QString KSingleShotString ( "SingleShot" );

const QString KBackwardString   (   "Backward" );
const QString KForwardString    (    "Forward" );

/*****************************************************************************
 * Initialization
 *****************************************************************************/

Function::Function(Doc* doc, Type t)
    : QObject(doc)
    , m_id(Function::invalidId())
    , m_type(t)
    , m_runOrder(Loop)
    , m_direction(Forward)
    , m_fadeInSpeed(0)
    , m_fadeOutSpeed(0)
    , m_duration(0)
    , m_overrideFadeInSpeed(defaultSpeed())
    , m_overrideFadeOutSpeed(defaultSpeed())
    , m_overrideDuration(defaultSpeed())
    , m_flashing(false)
    , m_elapsed(0)
    , m_stop(true)
    , m_running(false)
    , m_startedAsChild(false)
    , m_intensity(1.0)
{
    Q_ASSERT(doc != NULL);
}

Function::~Function()
{
}

Doc* Function::doc() const
{
    Doc* doc = qobject_cast<Doc*> (parent());
    return doc;
}

/*****************************************************************************
 * Copying
 *****************************************************************************/

bool Function::copyFrom(const Function* function)
{
    if (function == NULL)
        return false;

    m_name = function->name();
    m_runOrder = function->runOrder();
    m_direction = function->direction();
    m_fadeInSpeed = function->fadeInSpeed();
    m_fadeOutSpeed = function->fadeOutSpeed();
    m_duration = function->duration();

    emit changed(m_id);

    return true;
}

/*****************************************************************************
 * ID
 *****************************************************************************/

void Function::setID(quint32 id)
{
    /* Don't set doc modified status or emit changed signal, because this
       function is called only once during function creation. */
    m_id = id;
}

quint32 Function::id() const
{
    return m_id;
}

quint32 Function::invalidId()
{
    return UINT_MAX;
}

/*****************************************************************************
 * Name
 *****************************************************************************/

void Function::setName(const QString& name)
{
        m_name = QString(name);

    emit changed(m_id);
}

QString Function::name() const
{
    return m_name;
}

/*****************************************************************************
 * Type
 *****************************************************************************/

Function::Type Function::type() const
{
    return m_type;
}

QString Function::typeString() const
{
    return Function::typeToString(type());
}

QString Function::typeToString(Type type)
{
    switch (type)
    {
    case Scene:
        return KSceneString;
    case Chaser:
        return KChaserString;
    case EFX:
        return KEFXString;
    case Collection:
        return KCollectionString;
    case Script:
        return KScriptString;
    case RGBMatrix:
        return KRGBMatrixString;
    case Show:
        return KShowString;
    case Audio:
        return KAudioString;
    case Undefined:
    default:
        return KUndefinedString;
    }
}

Function::Type Function::stringToType(const QString& string)
{
    if (string == KSceneString)
        return Scene;
    else if (string == KChaserString)
        return Chaser;
    else if (string == KEFXString)
        return EFX;
    else if (string == KCollectionString)
        return Collection;
    else if (string == KScriptString)
        return Script;
    else if (string == KRGBMatrixString)
        return RGBMatrix;
    else if (string == KShowString)
        return Show;
    else if (string == KAudioString)
        return Audio;
    else
        return Undefined;
}

/*****************************************************************************
 * Running order
 *****************************************************************************/

void Function::setRunOrder(const Function::RunOrder& order)
{
    if (order == Loop || order == SingleShot || order == PingPong)
        m_runOrder = order;
    else
        m_runOrder = Loop;
    emit changed(m_id);
}

Function::RunOrder Function::runOrder() const
{
    return m_runOrder;
}

QString Function::runOrderToString(const RunOrder& order)
{
    switch (order)
    {
    default:
    case Loop:
        return KLoopString;
        break;

    case PingPong:
        return KPingPongString;
        break;

    case SingleShot:
        return KSingleShotString;
        break;
    }
}

Function::RunOrder Function::stringToRunOrder(const QString& str)
{
    if (str == KPingPongString)
        return PingPong;
    else if (str == KSingleShotString)
        return SingleShot;
    else
        return Loop;
}

bool Function::saveXMLRunOrder(QDomDocument* doc, QDomElement* root) const
{
    Q_ASSERT(doc != NULL);
    Q_ASSERT(root != NULL);

    QDomElement tag = doc->createElement(KXMLQLCFunctionRunOrder);
    root->appendChild(tag);
    QDomText text = doc->createTextNode(runOrderToString(runOrder()));
    tag.appendChild(text);

    return true;
}

bool Function::loadXMLRunOrder(const QDomElement& root)
{
    if (root.tagName() != KXMLQLCFunctionRunOrder)
    {
        qWarning() << Q_FUNC_INFO << "RunOrder node not found";
        return false;
    }

    setRunOrder(stringToRunOrder(root.text()));

    return true;
}

/*****************************************************************************
 * Direction
 *****************************************************************************/

void Function::setDirection(const Function::Direction& dir)
{
    if (dir == Forward || dir == Backward)
        m_direction = dir;
    else
        m_direction = Forward;
    emit changed(m_id);
}

Function::Direction Function::direction() const
{
    return m_direction;
}

QString Function::directionToString(const Direction& dir)
{
    switch (dir)
    {
    default:
    case Forward:
        return KForwardString;
        break;

    case Backward:
        return KBackwardString;
        break;
    }
}

Function::Direction Function::stringToDirection(const QString& str)
{
    if (str == KBackwardString)
        return Backward;
    else
        return Forward;
}

bool Function::saveXMLDirection(QDomDocument* doc, QDomElement* root) const
{
    Q_ASSERT(doc != NULL);
    Q_ASSERT(root != NULL);

    QDomElement tag = doc->createElement(KXMLQLCFunctionDirection);
    root->appendChild(tag);
    QDomText text = doc->createTextNode(directionToString(direction()));
    tag.appendChild(text);

    return true;
}

bool Function::loadXMLDirection(const QDomElement& root)
{
    if (root.tagName() != KXMLQLCFunctionDirection)
    {
        qWarning() << Q_FUNC_INFO << "Direction node not found";
        return false;
    }

    setDirection(stringToDirection(root.text()));

    return true;
}

/****************************************************************************
 * Speed
 ****************************************************************************/

void Function::setFadeInSpeed(uint ms)
{
    m_fadeInSpeed = ms;
    emit changed(m_id);
}

uint Function::fadeInSpeed() const
{
    return m_fadeInSpeed;
}

void Function::setFadeOutSpeed(uint ms)
{
    m_fadeOutSpeed = ms;
    emit changed(m_id);
}

uint Function::fadeOutSpeed() const
{
    return m_fadeOutSpeed;
}

void Function::setDuration(uint ms)
{
    m_duration = ms;
    emit changed(m_id);
}

uint Function::duration() const
{
    return m_duration;
}

void Function::setOverrideFadeInSpeed(uint ms)
{
    m_overrideFadeInSpeed = ms;
}

uint Function::overrideFadeInSpeed() const
{
    return m_overrideFadeInSpeed;
}

void Function::setOverrideFadeOutSpeed(uint ms)
{
    m_overrideFadeOutSpeed = ms;
}

uint Function::overrideFadeOutSpeed() const
{
    return m_overrideFadeOutSpeed;
}

void Function::setOverrideDuration(uint ms)
{
    m_overrideDuration = ms;
}

uint Function::overrideDuration() const
{
    return m_overrideDuration;
}

QString Function::speedToString(uint ms)
{
    QString str;
    if (ms == infiniteSpeed())
    {
        str = QChar(0x221E); // Infinity symbol
    }
    else
    {
        uint h, m, s;

        h = ms / MS_PER_HOUR;
        ms -= (h * MS_PER_HOUR);

        m = ms / MS_PER_MINUTE;
        ms -= (m * MS_PER_MINUTE);

        s = ms / MS_PER_SECOND;
        ms -= (s * MS_PER_SECOND);

        if (h != 0)
            str += QString("%1h").arg(h, 2, 10, QChar('0'));
        if (m != 0)
            str += QString("%1m").arg(m, 2, 10, QChar('0'));
        if (s != 0)
            str += QString("%1s").arg(s, 2, 10, QChar('0'));
        // Always display .ms
        str += QString(".%1").arg(ms / 10, 2, 10, QChar('0'));
    }

    return str;
}

uint Function::stringToSpeed(QString speed)
{
    uint value = 0;

    QStringList msecs = speed.split(".");
    if (msecs.count() > 0)
        value += (msecs.at(msecs.count() - 1).toUInt() * 10);

    QStringList secs = speed.split("s");
    if (secs.count() > 1)
        value += (secs.at(0).toUInt() * 1000);

    QStringList mins = speed.split("m");
    if (mins.count() > 1)
        value += (mins.at(0).toUInt() * 60 * 1000);

    QStringList hours = speed.split("h");
    if (hours.count() > 1)
        value += (hours.at(0).toUInt() * 60 * 60 * 1000);

    return value;
}

void Function::tap()
{
}

bool Function::loadXMLSpeed(const QDomElement& speedRoot)
{
    if (speedRoot.tagName() != KXMLQLCFunctionSpeed)
        return false;

    m_fadeInSpeed = speedRoot.attribute(KXMLQLCFunctionSpeedFadeIn).toUInt();
    m_fadeOutSpeed = speedRoot.attribute(KXMLQLCFunctionSpeedFadeOut).toUInt();
    m_duration = speedRoot.attribute(KXMLQLCFunctionSpeedDuration).toUInt();

    return true;
}

bool Function::saveXMLSpeed(QDomDocument* doc, QDomElement* root) const
{
    QDomElement tag;

    tag = doc->createElement(KXMLQLCFunctionSpeed);
    tag.setAttribute(KXMLQLCFunctionSpeedFadeIn, QString::number(fadeInSpeed()));
    tag.setAttribute(KXMLQLCFunctionSpeedFadeOut, QString::number(fadeOutSpeed()));
    tag.setAttribute(KXMLQLCFunctionSpeedDuration, QString::number(duration()));
    root->appendChild(tag);

    return true;
}

uint Function::infiniteSpeed()
{
    return (uint) -2;
}

uint Function::defaultSpeed()
{
    return (uint) -1;
}

/*****************************************************************************
 * Fixtures
 *****************************************************************************/

void Function::slotFixtureRemoved(quint32 fid)
{
    Q_UNUSED(fid);
}

/*****************************************************************************
 * Load & Save
 *****************************************************************************/

bool Function::loader(const QDomElement& root, Doc* doc)
{
    if (root.tagName() != KXMLQLCFunction)
    {
        qWarning("Function node not found!");
        return false;
    }

    /* Get common information from the tag's attributes */
    quint32 id = root.attribute(KXMLQLCFunctionID).toInt();
    QString name = root.attribute(KXMLQLCFunctionName);
    Type type = Function::stringToType(root.attribute(KXMLQLCFunctionType));

    /* Check for ID validity before creating the function */
    if (id == Function::invalidId())
    {
        qWarning() << Q_FUNC_INFO << "Function ID" << id << "is not allowed.";
        return false;
    }

    /* Create a new function according to the type */
    Function* function = NULL;
    if (type == Function::Scene)
        function = new class Scene(doc);
    else if (type == Function::Chaser)
        function = new class Chaser(doc);
    else if (type == Function::Collection)
        function = new class Collection(doc);
    else if (type == Function::EFX)
        function = new class EFX(doc);
    else if (type == Function::Script)
        function = new class Script(doc);
    else if (type == Function::RGBMatrix)
        function = new class RGBMatrix(doc);
    else if (type == Function::Show)
        function = new class Show(doc);
    else if (type == Function::Audio)
        function = new class Audio(doc);
    else
        return false;

    function->setName(name);
    if (function->loadXML(root) == true)
    {
        if (doc->addFunction(function, id) == true)
        {
            /* Success */
            return true;
        }
        else
        {
            qWarning() << "Function" << name << "cannot be created.";
            delete function;
            return false;
        }
    }
    else
    {
        qWarning() << "Function" << name << "cannot be loaded.";
        delete function;
        return false;
    }
}

void Function::postLoad()
{
    /* NOP */
}

/*****************************************************************************
 * Flash
 *****************************************************************************/

void Function::flash(MasterTimer* timer)
{
    Q_UNUSED(timer);
    if (m_flashing == false)
        emit flashing(m_id, true);
    m_flashing = true;
}

void Function::unFlash(MasterTimer* timer)
{
    Q_UNUSED(timer);
    if (m_flashing == true)
        emit flashing(m_id, false);
    m_flashing = false;
}

bool Function::flashing() const
{
    return m_flashing;
}

/*****************************************************************************
 * Running
 *****************************************************************************/

void Function::preRun(MasterTimer* timer)
{
    Q_UNUSED(timer);

    qDebug() << "Function preRun. ID: " << m_id;
    m_stop = false;
    m_running = true;

    emit running(m_id);
}

void Function::postRun(MasterTimer* timer, UniverseArray* universes)
{
    Q_UNUSED(timer);
    Q_UNUSED(universes);

    qDebug() << "Function postRun. ID: " << m_id;
    m_stopMutex.lock();
    resetElapsed();
    resetIntensity();
    m_stop = true;
    //m_overrideFadeInSpeed = defaultSpeed();
    //m_overrideFadeOutSpeed = defaultSpeed();
    //m_overrideDuration = defaultSpeed();
    m_functionStopped.wakeAll();
    m_stopMutex.unlock();

    m_running = false;
    emit stopped(m_id);
}

bool Function::isRunning() const
{
    return m_running;
}

/*****************************************************************************
 * Elapsed ticks while running
 *****************************************************************************/

quint32 Function::elapsed() const
{
    return m_elapsed;
}

void Function::resetElapsed()
{
    qDebug() << Q_FUNC_INFO;
    m_elapsed = 0;
}

void Function::incrementElapsed()
{
    // Don't wrap around. UINT_MAX is the maximum fade/hold time.
    if (m_elapsed < UINT_MAX)
        m_elapsed += MasterTimer::tick();
}

/*****************************************************************************
 * Start & Stop
 *****************************************************************************/

void Function::start(MasterTimer* timer, bool child, uint overrideFadeIn,
                     uint overrideFadeOut, uint overrideDuration)
{
    qDebug() << "Function start(). ID: " << m_id;
    Q_ASSERT(timer != NULL);
    m_startedAsChild = child;
    m_overrideFadeInSpeed = overrideFadeIn;
    m_overrideFadeOutSpeed = overrideFadeOut;
    m_overrideDuration = overrideDuration;
    timer->startFunction(this);
}

bool Function::startedAsChild() const
{
    return m_startedAsChild;
}

void Function::stop()
{
    qDebug() << "Function stop(). ID: " << m_id;
    m_stop = true;
}

bool Function::stopped() const
{
    return m_stop;
}

bool Function::stopAndWait()
{
    bool result = true;

    m_stopMutex.lock();
    m_stop = true;

    QTime watchdog;
    watchdog.start();

    // block thread for maximum 2 seconds
    while (m_elapsed > 0)
    {
        if (watchdog.elapsed() > 2000)
        {
              result = false;
              break;
        }

        // wait until the function has stopped
        m_functionStopped.wait(&m_stopMutex, 100);
    }

    m_stopMutex.unlock();
    return result;
}

/*****************************************************************************
 * Intensity
 *****************************************************************************/

void Function::adjustIntensity(qreal fraction)
{
    m_intensity = CLAMP(fraction, 0.0, 1.0);
    emit intensityChanged(m_intensity);
}

void Function::resetIntensity()
{
    adjustIntensity(1.0);
}

qreal Function::intensity() const
{
    return m_intensity;
}
