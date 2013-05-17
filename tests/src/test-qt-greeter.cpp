#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <glib-object.h>
#include <xcb/xcb.h>
#include <QLightDM/Greeter>
#include <QLightDM/Power>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

#include "test-qt-greeter.h"
#include "status.h"

static gchar *greeter_id;
static QCoreApplication *app = NULL;
static QSettings *config = NULL;
static QLightDM::PowerInterface *power = NULL;
static TestGreeter *greeter = NULL;

TestGreeter::TestGreeter ()
{
    connect (this, SIGNAL(showMessage(QString, QLightDM::Greeter::MessageType)), SLOT(showMessage(QString, QLightDM::Greeter::MessageType)));
    connect (this, SIGNAL(showPrompt(QString, QLightDM::Greeter::PromptType)), SLOT(showPrompt(QString, QLightDM::Greeter::PromptType)));
    connect (this, SIGNAL(authenticationComplete()), SLOT(authenticationComplete()));
    connect (this, SIGNAL(autologinTimerExpired()), SLOT(autologinTimerExpired()));
}

void TestGreeter::showMessage (QString text, QLightDM::Greeter::MessageType type)
{
    status_notify ("%s SHOW-MESSAGE TEXT=\"%s\"", greeter_id, text.toAscii ().constData ());
}

void TestGreeter::showPrompt (QString text, QLightDM::Greeter::PromptType type)
{
    status_notify ("%s SHOW-PROMPT TEXT=\"%s\"", greeter_id, text.toAscii ().constData ());
}

void TestGreeter::authenticationComplete ()
{
    if (authenticationUser () != "")
        status_notify ("%s AUTHENTICATION-COMPLETE USERNAME=%s AUTHENTICATED=%s",
                       greeter_id,
                       authenticationUser ().toAscii ().constData (), isAuthenticated () ? "TRUE" : "FALSE");
    else
        status_notify ("%s AUTHENTICATION-COMPLETE AUTHENTICATED=%s", greeter_id, isAuthenticated () ? "TRUE" : "FALSE");
}

void TestGreeter::autologinTimerExpired ()
{
    status_notify ("%s AUTOLOGIN-TIMER-EXPIRED", greeter_id);
}

static void
signal_cb (int signum)
{
    status_notify ("%s TERMINATE SIGNAL=%d", greeter_id, signum);
    _exit (EXIT_SUCCESS);
}

static void
request_cb (const gchar *request)
{
    gchar *r;

    if (!request)
    {
        app->quit ();
        return;
    }
  
    r = g_strdup_printf ("%s AUTHENTICATE", greeter_id);
    if (strcmp (request, r) == 0)
        greeter->authenticate ();
    g_free (r);

    r = g_strdup_printf ("%s AUTHENTICATE USERNAME=", greeter_id);
    if (g_str_has_prefix (request, r))
        greeter->authenticate (request + strlen (r));
    g_free (r);

    r = g_strdup_printf ("%s AUTHENTICATE-GUEST", greeter_id);
    if (strcmp (request, r) == 0)
        greeter->authenticateAsGuest ();
    g_free (r);

    r = g_strdup_printf ("%s AUTHENTICATE-AUTOLOGIN", greeter_id);
    if (strcmp (request, r) == 0)
        greeter->authenticateAutologin ();
    g_free (r);

    r = g_strdup_printf ("%s AUTHENTICATE-REMOTE SESSION=", greeter_id);
    if (g_str_has_prefix (request, r))
        greeter->authenticateRemote (request + strlen (r), NULL);
    g_free (r);

    r = g_strdup_printf ("%s RESPOND TEXT=\"", greeter_id);
    if (g_str_has_prefix (request, r))
    {
        gchar *text = g_strdup (request + strlen (r));
        text[strlen (text) - 1] = '\0';
        greeter->respond (text);
        g_free (text);
    }
    g_free (r);

    r = g_strdup_printf ("%s CANCEL-AUTHENTICATION", greeter_id);
    if (strcmp (request, r) == 0)
        greeter->cancelAuthentication ();
    g_free (r);

    r = g_strdup_printf ("%s START-SESSION", greeter_id);
    if (strcmp (request, r) == 0)
    {
        if (!greeter->startSessionSync ())
            status_notify ("%s SESSION-FAILED", greeter_id);
    }
    g_free (r);

    r = g_strdup_printf ("%s START-SESSION SESSION=", greeter_id);
    if (g_str_has_prefix (request, r))
    {
        if (!greeter->startSessionSync (request + strlen (r)))
            status_notify ("%s SESSION-FAILED", greeter_id);
    }
    g_free (r);

    r = g_strdup_printf ("%s GET-CAN-SUSPEND", greeter_id);
    if (strcmp (request, r) == 0)
    {
        gboolean can_suspend = power->canSuspend ();
        status_notify ("%s CAN-SUSPEND ALLOWED=%s", greeter_id, can_suspend ? "TRUE" : "FALSE");
    }
    g_free (r);

    r = g_strdup_printf ("%s SUSPEND", greeter_id);
    if (strcmp (request, r) == 0)
    {
        if (!power->suspend ())
            status_notify ("%s FAIL-SUSPEND", greeter_id);
    }
    g_free (r);

    r = g_strdup_printf ("%s GET-CAN-HIBERNATE", greeter_id);
    if (strcmp (request, r) == 0)
    {
        gboolean can_hibernate = power->canHibernate ();
        status_notify ("%s CAN-HIBERNATE ALLOWED=%s", greeter_id, can_hibernate ? "TRUE" : "FALSE");
    }
    g_free (r);

    r = g_strdup_printf ("%s HIBERNATE", greeter_id);
    if (strcmp (request, r) == 0)
    {
        if (!power->hibernate ())
            status_notify ("%s FAIL-HIBERNATE", greeter_id);
    }
    g_free (r);

    r = g_strdup_printf ("%s GET-CAN-RESTART", greeter_id);
    if (strcmp (request, r) == 0)
    {
        gboolean can_restart = power->canRestart ();
        status_notify ("%s CAN-RESTART ALLOWED=%s", greeter_id, can_restart ? "TRUE" : "FALSE");
    }
    g_free (r);

    r = g_strdup_printf ("%s RESTART", greeter_id);
    if (strcmp (request, r) == 0)
    {
        if (!power->restart ())
            status_notify ("%s FAIL-RESTART", greeter_id);
    }
    g_free (r);

    r = g_strdup_printf ("%s GET-CAN-SHUTDOWN", greeter_id);
    if (strcmp (request, r) == 0)
    {
        gboolean can_shutdown = power->canShutdown ();
        status_notify ("%s CAN-SHUTDOWN ALLOWED=%s", greeter_id, can_shutdown ? "TRUE" : "FALSE");
    }
    g_free (r);

    r = g_strdup_printf ("%s SHUTDOWN", greeter_id);
    if (strcmp (request, r) == 0)
    {
        if (!power->shutdown ())
            status_notify ("%s FAIL-SHUTDOWN", greeter_id);
    }
    g_free (r);
}

int
main(int argc, char *argv[])
{
    gchar *display;

#if !defined(GLIB_VERSION_2_36)
    g_type_init ();
#endif

    display = getenv ("DISPLAY");
    if (display == NULL)
        greeter_id = g_strdup ("GREETER-?");
    else if (display[0] == ':')
        greeter_id = g_strdup_printf ("GREETER-X-%s", display + 1);
    else
        greeter_id = g_strdup_printf ("GREETER-X-%s", display);

    status_connect (request_cb);

    app = new QCoreApplication (argc, argv);

    signal (SIGINT, signal_cb);
    signal (SIGTERM, signal_cb);

    status_notify ("%s START", greeter_id);

    config = new QSettings (g_build_filename (getenv ("LIGHTDM_TEST_ROOT"), "script", NULL), QSettings::IniFormat);

    xcb_connection_t *connection = xcb_connect (NULL, NULL);

    if (xcb_connection_has_error (connection))
    {
        status_notify ("%s FAIL-CONNECT-XSERVER", greeter_id);
        return EXIT_FAILURE;
    }

    status_notify ("%s CONNECT-XSERVER", greeter_id);

    power = new QLightDM::PowerInterface();

    greeter = new TestGreeter();
  
    status_notify ("%s CONNECT-TO-DAEMON", greeter_id);
    if (!greeter->connectSync())
    {
        status_notify ("%s FAIL-CONNECT-DAEMON", greeter_id);
        return EXIT_FAILURE;
    }

    status_notify ("%s CONNECTED-TO-DAEMON", greeter_id);

    if (greeter->selectUserHint() != "")
        status_notify ("%s SELECT-USER-HINT USERNAME=%s", greeter_id, greeter->selectUserHint ().toAscii ().constData ());
    if (greeter->lockHint())
        status_notify ("%s LOCK-HINT", greeter_id);

    return app->exec();
}
