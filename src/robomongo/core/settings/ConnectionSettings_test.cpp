#include "gtest/gtest.h"
#include "ConnectionSettings.h"

#include <memory>

using Robomongo::ConnectionSettings;

TEST(connection_settings_tests, AppName_Default_IsEmpty)
{
    ConnectionSettings settings(false);
    EXPECT_TRUE(settings.appName().empty());
}

TEST(connection_settings_tests, AppName_SavedAndLoaded_RoundTrips)
{
    ConnectionSettings original(false);
    original.setAppName("billing service");

    ConnectionSettings loaded(false);
    loaded.fromVariant(original.toVariant().toMap());
    EXPECT_EQ("billing service", loaded.appName());
}

TEST(connection_settings_tests, AppName_MissingFromOldConfig_LoadsEmpty)
{
    ConnectionSettings original(false);
    QVariantMap map = original.toVariant().toMap();
    map.remove("appName");

    ConnectionSettings loaded(false);
    loaded.fromVariant(map);
    EXPECT_TRUE(loaded.appName().empty());
}

TEST(connection_settings_tests, EffectiveAppName_PrefersConnectionThenDefaultThenFallback)
{
    ConnectionSettings settings(false);
    EXPECT_EQ("fallback", settings.effectiveAppName("", "fallback"));
    EXPECT_EQ("application", settings.effectiveAppName("application", "fallback"));

    settings.setAppName("billing service");
    EXPECT_EQ("billing service", settings.effectiveAppName("application", "fallback"));
}

TEST(connection_settings_tests, Namespace_SavedLoadedAndCloned)
{
    ConnectionSettings original(false);
    original.setConnectionNamespace("prod");

    ConnectionSettings loaded(false);
    loaded.fromVariant(original.toVariant().toMap());
    EXPECT_EQ("prod", loaded.connectionNamespace());

    std::unique_ptr<ConnectionSettings> clone(original.clone());
    EXPECT_EQ("prod", clone->connectionNamespace());
}

TEST(connection_settings_tests, AppName_Clone_CopiesValue)
{
    ConnectionSettings original(false);
    original.setAppName("reporting");

    std::unique_ptr<ConnectionSettings> clone(original.clone());
    EXPECT_EQ("reporting", clone->appName());
}
