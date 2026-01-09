#define G_LOG_DOMAIN "RBW"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <gmodule.h>
#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>
#include <glib.h>

G_MODULE_EXPORT Mode mode;

// Entry structure
typedef struct {
    char *name;
    char *username;
    char *folder;
} RBWEntry;

// Plugin private data
typedef struct {
    RBWEntry *entries;
    int num_entries;
    int entry_capacity;
    bool edit_mode;
    bool settings_mode;
    int selected_entry_idx;
} RBWModePrivateData;

// Key types for detection
typedef enum {
    KEY_NONE,
    KEY_ENTER,
    KEY_CUSTOM_ACTION
} RBWKey;

// Function declarations
static int rbw_init(Mode *sw);
static unsigned int rbw_get_num_entries(const Mode *sw);
static ModeMode rbw_result(Mode *sw, int mretv, char **input, unsigned int selected_line);
static void rbw_destroy(Mode *sw);
static char* rbw_get_display_value(const Mode *sw, unsigned int index, int *state, GList **attr_list, int get_entry);
static int rbw_token_match(const Mode *sw, rofi_int_matcher **tokens, unsigned int index);
static char* rbw_get_message(const Mode *sw);

// Load entries from rbw
static void load_entries(RBWModePrivateData *pd) {
    FILE *fp = popen("rbw list --fields name,user,folder 2>/dev/null", "r");
    if (!fp) return;
    
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Parse: name\tuser\tfolder
        char *name = line;
        char *tab1 = strchr(line, '\t');
        if (!tab1) continue;
        *tab1 = '\0';
        
        char *username = tab1 + 1;
        char *tab2 = strchr(username, '\t');
        char *folder = "";
        if (tab2) {
            *tab2 = '\0';
            folder = tab2 + 1;
        }
        
        // Add entry
        if (pd->num_entries >= pd->entry_capacity) {
            pd->entry_capacity = pd->entry_capacity == 0 ? 10 : pd->entry_capacity * 2;
            pd->entries = g_realloc(pd->entries, pd->entry_capacity * sizeof(RBWEntry));
        }
        
        pd->entries[pd->num_entries].name = g_strdup(name);
        pd->entries[pd->num_entries].username = g_strdup(username);
        pd->entries[pd->num_entries].folder = g_strdup(folder);
        pd->num_entries++;
    }
    
    pclose(fp);
}

// Helper macros
#define IS_CUSTOM_KEY(m) ((m) & MENU_CUSTOM_ACTION)
#define IS_ENTER(m) ((m) & MENU_OK)
#define IS_CANCEL(m) ((m) & MENU_CANCEL)
#define RUN_CMD(cmd) do { system(cmd " &"); } while(0)
#define RUN_HELPER(action, ...) do { \
    char *h = g_find_program_in_path("rofi-bitwarden-helper") ?: g_strdup("/usr/local/bin/rofi-bitwarden-helper"); \
    char *c = g_strdup_printf("'%s' " action, h, ##__VA_ARGS__); \
    system(c); g_free(c); g_free(h); \
} while(0)

// Initialize mode
static int rbw_init(Mode *sw) {
    if (mode_get_private_data(sw) != NULL) {
        return true;
    }
    
    RBWModePrivateData *pd = g_malloc0(sizeof(*pd));
    mode_set_private_data(sw, pd);
    
    pd->edit_mode = false;
    pd->settings_mode = false;
    pd->selected_entry_idx = -1;
    
    // Load entries
    load_entries(pd);
    
    return true;
}

// Cleanup and destroy
static void rbw_destroy(Mode *sw) {
    RBWModePrivateData *pd = (RBWModePrivateData *)mode_get_private_data(sw);
    if (!pd) return;
    
    // Free entries
    for (int i = 0; i < pd->num_entries; i++) {
        g_free(pd->entries[i].name);
        g_free(pd->entries[i].username);
        g_free(pd->entries[i].folder);
    }
    g_free(pd->entries);
    g_free(pd);
    
    mode_set_private_data(sw, NULL);
}

// Get number of entries
static unsigned int rbw_get_num_entries(const Mode *sw) {
    const RBWModePrivateData *pd = (const RBWModePrivateData *)mode_get_private_data(sw);
    
    if (pd->edit_mode) {
        return 8; // Copy Password, Copy Username, Open URL, Edit Password, Edit Username, Edit URL, Type Password, Delete
    } else if (pd->settings_mode) {
        return 4; // Add Entry, Generate Password, Sync, Lock
    } else {
        return pd->num_entries + 1; // +1 for "Settings"
    }
}

// Handle result selection
static ModeMode rbw_result(Mode *sw, int mretv, char **input, unsigned int selected_line) {
    RBWModePrivateData *pd = (RBWModePrivateData *)mode_get_private_data(sw);
    
    // Handle edit mode
    if (pd->edit_mode) {
        if (mretv & MENU_OK) {
            if (pd->selected_entry_idx >= 0 && pd->selected_entry_idx < pd->num_entries) {
                RBWEntry *entry = &pd->entries[pd->selected_entry_idx];
                char *helper = g_find_program_in_path("rofi-rbw-helper");
                if (!helper) {
                    helper = g_strdup("/usr/local/bin/rofi-rbw-helper");
                }
                
                if (selected_line == 0) { // Copy Password
                    char *cmd = g_strdup_printf("rbw get '%s' 2>/dev/null | tr -d '\\n' | xclip -selection clipboard && notify-send 'RBW' 'Password copied' &", entry->name);
                    system(cmd);
                    g_free(cmd);
                    g_free(helper);
                    return MODE_EXIT;
                } else if (selected_line == 1) { // Copy Username
                    char *cmd = g_strdup_printf("echo -n '%s' | xclip -selection clipboard && notify-send 'RBW' 'Username copied' &", entry->username);
                    system(cmd);
                    g_free(cmd);
                    g_free(helper);
                    return MODE_EXIT;
                } else if (selected_line == 2) { // Open URL
                    char *cmd = g_strdup_printf("bash -c 'URL=$(rbw get --field uris \"%s\" 2>/dev/null); if [ -n \"$URL\" ]; then xdg-open \"$URL\"; else notify-send \"RBW\" \"No URL found\"; fi' &", entry->name);
                    system(cmd);
                    g_free(cmd);
                    g_free(helper);
                    return MODE_EXIT;
                } else if (selected_line == 3) { // Edit Password
                    char *cmd = g_strdup_printf("'%s' edit-password '%s' &", helper, entry->name);
                    system(cmd);
                    g_free(cmd);
                    g_free(helper);
                    return MODE_EXIT;
                } else if (selected_line == 4) { // Edit Username
                    char *cmd = g_strdup_printf("'%s' edit-username '%s' '%s' &", helper, entry->name, entry->username);
                    system(cmd);
                    g_free(cmd);
                    g_free(helper);
                    return MODE_EXIT;
                } else if (selected_line == 5) { // Edit URL
                    char *cmd = g_strdup_printf("'%s' edit-url '%s' &", helper, entry->name);
                    system(cmd);
                    g_free(cmd);
                    g_free(helper);
                    return MODE_EXIT;
                } else if (selected_line == 6) { // Type Password
                    char *cmd = g_strdup_printf("sleep 0.5 && rbw get '%s' 2>/dev/null | tr -d '\\n' | xdotool type --clearmodifiers --delay 100 --file - &", entry->name);
                    system(cmd);
                    g_free(cmd);
                    g_free(helper);
                    return MODE_EXIT;
                } else if (selected_line == 7) { // Delete
                    char *cmd = g_strdup_printf("'%s' delete '%s' &", helper, entry->name);
                    system(cmd);
                    g_free(cmd);
                    g_free(helper);
                    return MODE_EXIT;
                }
                g_free(helper);
            }
            pd->edit_mode = false;
            pd->selected_entry_idx = -1;
            return RESET_DIALOG;
        } else if (mretv & MENU_CANCEL) {
            pd->edit_mode = false;
            pd->selected_entry_idx = -1;
            return RESET_DIALOG;
        }
    }
    
    // Handle settings mode
    if (pd->settings_mode) {
        if (mretv & MENU_OK) {
            if (selected_line == 0) { // Add Entry
                char *helper = g_find_program_in_path("rofi-bitwarden-helper");
                if (!helper) {
                    helper = g_strdup("/usr/local/bin/rofi-bitwarden-helper");
                }
                char *cmd = g_strdup_printf("bash -c '\"'%s'\" add && sleep 0.5 && rofi -show rbw -modi rbw' &", helper);
                system(cmd);
                g_free(cmd);
                g_free(helper);
                return MODE_EXIT;
            } else if (selected_line == 1) { // Generate Password
                system("bash -c 'PASSWORD=$(rbw generate 20 2>/dev/null); if [ -n \"$PASSWORD\" ]; then echo -n \"$PASSWORD\" | xclip -selection clipboard && notify-send \"RBW\" \"Generated password copied to clipboard\"; else notify-send \"RBW\" \"Failed to generate password\"; fi' &");
                return MODE_EXIT;
            } else if (selected_line == 2) { // Sync
                system("(rbw sync && notify-send 'RBW' 'Synced with server') &");
                return MODE_EXIT;
            } else if (selected_line == 3) { // Lock
                system("(rbw lock && notify-send 'RBW' 'Vault locked') &");
                return MODE_EXIT;
            }
        } else if (mretv & MENU_CANCEL) {
            pd->settings_mode = false;
            return RESET_DIALOG;
        }
    }
    
    // Handle kb-accept-alt (Shift+Return) in main mode - show action menu
    if ((mretv & MENU_CUSTOM_ACTION) && selected_line > 0) {
        pd->edit_mode = true;
        pd->selected_entry_idx = selected_line - 1;
        return RESET_DIALOG;
    }
    
    // Handle main mode - Enter key
    if (mretv & MENU_OK) {
        if (selected_line == 0) {
            // Open Settings submenu
            pd->settings_mode = true;
            return RESET_DIALOG;
        } else {
            // Copy password to clipboard
            int idx = selected_line - 1;
            if (idx >= 0 && idx < pd->num_entries) {
                char *cmd = g_strdup_printf("rbw get '%s' 2>/dev/null | tr -d '\\n' | xclip -selection clipboard && notify-send 'RBW' 'Password copied to clipboard' &", pd->entries[idx].name);
                system(cmd);
                g_free(cmd);
            }
            return MODE_EXIT;
        }
    } else if (mretv & MENU_CANCEL) {
        return MODE_EXIT;
    }
    
    return RELOAD_DIALOG;
}

// Token matching
static int rbw_token_match(const Mode *sw, rofi_int_matcher **tokens, unsigned int index) {
    const RBWModePrivateData *pd = (const RBWModePrivateData *)mode_get_private_data(sw);
    
    if (pd->edit_mode) {
        const char *options[] = {"Copy Password", "Copy Username", "Open URL", "Edit Password", "Edit Username", "Edit URL", "Type Password", "Delete"};
        return helper_token_match(tokens, options[index]);
    } else if (pd->settings_mode) {
        const char *options[] = {"Add Entry", "Generate Password", "Sync", "Lock"};
        return helper_token_match(tokens, options[index]);
    } else {
        if (index == 0) {
            return helper_token_match(tokens, "Settings");
        } else {
            int idx = index - 1;
            char *search_str = g_strdup_printf("%s %s %s", 
                pd->entries[idx].name,
                pd->entries[idx].username,
                pd->entries[idx].folder);
            int result = helper_token_match(tokens, search_str);
            g_free(search_str);
            return result;
        }
    }
}

// Get display value
static char* rbw_get_display_value(const Mode *sw, unsigned int index, int *state, G_GNUC_UNUSED GList **attr_list, int get_entry) {
    const RBWModePrivateData *pd = (const RBWModePrivateData *)mode_get_private_data(sw);
    
    if (pd->edit_mode) {
        const char *options[] = {"🔑 Copy Password", "👤 Copy Username", "🌐 Open URL", "✏️ Edit Password", "✏️ Edit Username", "🔗 Edit URL", "⌨️ Type Password", "🗑️ Delete"};
        return get_entry ? g_strdup(options[index]) : NULL;
    } else if (pd->settings_mode) {
        const char *options[] = {"➕ Add Entry", "🎲 Generate Password", "🔄 Sync", "🔒 Lock"};
        return get_entry ? g_strdup(options[index]) : NULL;
    } else {
        if (index == 0) {
            return get_entry ? g_strdup("⚙️ Settings") : NULL;
        } else {
            int idx = index - 1;
            if (idx < pd->num_entries) {
                if (pd->entries[idx].folder && strlen(pd->entries[idx].folder) > 0) {
                    return get_entry ? g_strdup_printf("🔐 [%s] %s (%s)", 
                        pd->entries[idx].folder,
                        pd->entries[idx].name, 
                        pd->entries[idx].username) : NULL;
                } else {
                    return get_entry ? g_strdup_printf("🔐 %s (%s)", 
                        pd->entries[idx].name, 
                        pd->entries[idx].username) : NULL;
                }
            }
        }
    }
    
    return NULL;
}

// Get message
static char* rbw_get_message(const Mode *sw) {
    const RBWModePrivateData *pd = (const RBWModePrivateData *)mode_get_private_data(sw);
    
    if (pd->edit_mode) {
        return g_strdup("Select action");
    } else if (pd->settings_mode) {
        return g_strdup("Select setting");
    } else {
        return g_strdup("Enter: Copy Password | Shift+Enter: Actions");
    }
}

// Mode definition
Mode mode = {
    .abi_version = ABI_VERSION,
    .name = "rbw",
    .cfg_name_key = "display-rbw",
    ._init = rbw_init,
    ._get_num_entries = rbw_get_num_entries,
    ._result = rbw_result,
    ._destroy = rbw_destroy,
    ._token_match = rbw_token_match,
    ._get_display_value = rbw_get_display_value,
    ._get_message = rbw_get_message,
    ._get_completion = NULL,
    ._preprocess_input = NULL,
    .private_data = NULL,
    .free = NULL
};
