# Aetherial Items Web Interface

A web application that displays items from the Aether JSON file in a Diablo-style card format. The application filters items to show only those with at least two wear flags.

## Features

- **Diablo-style Item Cards**: Beautiful, dark-themed item cards
- **Filtering**: Search by item name and filter by item type
- **Responsive Design**: Works on desktop and mobile devices

## How to Use

1. **Setup**: Ensure the `json/aether.json` file is in the `json/` directory relative to the app folder
2. **Run**: Start the server from the app directory: `cd app && python3 -m http.server 8000`
3. **Open**: Go to `http://localhost:8000` in your web browser
4. **Browse**: View all items
5. **Search**: Use the search box to find specific items
6. **Filter**: Use the dropdown to filter by item type (weapons, armor, jewelry, etc.)

## Item Information Displayed

Each item card shows:
- **Item Name**: The item's display name
- **Level**: Required level to use the item
- **Type**: Item category (weapon, armor, jewelry, etc.)
- **Statistics**: All stat modifiers and bonuses
- **Weapon Stats**: Damage dice and damage type for weapons
- **Armor Stats**: Armor class values for armor items
- **Wear Flags**: All wear locations and restrictions
- **Description**: Item description

## File Structure

```
├── index.html              # Main HTML file
├── styles.css              # CSS styling for Diablo theme
├── script.js               # JavaScript functionality
├── test.html               # Test page for JSON loading
├── README.md               # This file
└── json/                   # Generated item data
    └── aether.json         # Item data file
```

## Technical Details

- **Pure HTML/CSS/JavaScript**: No external dependencies required
- **Async Loading**: Items are loaded asynchronously from the JSON file
- **Responsive Grid**: Items are displayed in a responsive grid layout
- **Color-coded Stats**: Positive stats are green, negative are red, neutral are gray
- **Hover Effects**: Cards have smooth hover animations

## Browser Compatibility

Works in all modern browsers that support:
- ES6+ JavaScript features
- CSS Grid
- Fetch API
- CSS Custom Properties

## Troubleshooting

If items don't load:
1. Check that `json/aether.json` exists and is accessible
2. Ensure you're running the files through a web server (not just opening the HTML file directly)
3. Check browser console for any JavaScript errors

## Local Development

To run locally, you can use any simple HTTP server:

```bash
# Using Python 3
python -m http.server 8000

# Using Node.js (if you have http-server installed)
npx http-server

# Using PHP
php -S localhost:8000
```

Then open `http://localhost:8000` in your browser. 