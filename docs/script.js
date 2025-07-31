class ItemViewer {
    constructor() {
        this.items = [];
        this.filteredItems = [];
        this.init();
    }

    // Color parsing system for ANSI/extended color codes
    parseColorCodes(text) {
        if (!text || typeof text !== 'string') return text;

                 // Color code mappings
         const colorMap = {
             // Basic Colors (8-color ANSI) - More subtle versions
             '{n': { color: '#ffffff', bgColor: 'transparent' }, // Clear/Reset
             '{r': { color: '#cc6666', bgColor: 'transparent' }, // Dark Red
             '{g': { color: '#66cc66', bgColor: 'transparent' }, // Dark Green
             '{y': { color: '#cccc66', bgColor: 'transparent' }, // Dark Yellow
             '{b': { color: '#6666cc', bgColor: 'transparent' }, // Dark Blue
             '{m': { color: '#cc66cc', bgColor: 'transparent' }, // Dark Magenta
             '{c': { color: '#66cccc', bgColor: 'transparent' }, // Dark Cyan
             '{w': { color: '#cccccc', bgColor: 'transparent' }, // Dark White
             '{D': { color: '#666666', bgColor: 'transparent' }, // Dark Grey

             // Bright Colors (8-color ANSI) - More subtle versions
             '{R': { color: '#ff6666', bgColor: 'transparent' }, // Bright Red
             '{G': { color: '#66ff66', bgColor: 'transparent' }, // Bright Green
             '{Y': { color: '#ffff66', bgColor: 'transparent' }, // Bright Yellow
             '{B': { color: '#6666ff', bgColor: 'transparent' }, // Bright Blue
             '{M': { color: '#ff66ff', bgColor: 'transparent' }, // Bright Magenta
             '{C': { color: '#66ffff', bgColor: 'transparent' }, // Bright Cyan
             '{W': { color: '#ffffff', bgColor: 'transparent' }, // Bright White

             // Extended Colors (256-color ANSI) - More subtle versions
             '{n': { color: '#ffaa66', bgColor: 'transparent' }, // Orange
             '{N': { color: '#ffcc66', bgColor: 'transparent' }, // Bright Orange
             '{p': { color: '#aa66ff', bgColor: 'transparent' }, // Purple
             '{P': { color: '#cc9966', bgColor: 'transparent' }, // Bright Purple
             '{t': { color: '#66aaaa', bgColor: 'transparent' }, // Teal
             '{T': { color: '#66cccc', bgColor: 'transparent' }, // Bright Teal
             '{l': { color: '#66cc66', bgColor: 'transparent' }, // Lime
             '{L': { color: '#aaccaa', bgColor: 'transparent' }, // Bright Lime
             '{s': { color: '#aaaaaa', bgColor: 'transparent' }, // Slate
             '{S': { color: '#aacccc', bgColor: 'transparent' }, // Bright Slate

            // Special Formatting Codes
            '{@': { fontWeight: 'bold' }, // Bold
            '{!': { textDecoration: 'blink' }, // Blink (not supported in CSS, using animation)
            '{+': { filter: 'invert(1)' }, // Reverse
            '{x': { color: '#ffffff', bgColor: 'transparent' }, // Clear/Reset
            '{{': { content: '{' }, // Literal '{'
            '{-': { content: '~' }, // Literal '~'
        };

        // RGB color code pattern for modern protocol
        const rgbPattern = /\\t\[([FB])(\d{3})\]/g;

        let result = text;

        // Handle RGB color codes first
        result = result.replace(rgbPattern, (match, type, rgb) => {
            const r = parseInt(rgb[0]) * 51; // Convert 0-5 to 0-255
            const g = parseInt(rgb[1]) * 51;
            const b = parseInt(rgb[2]) * 51;
            const color = `rgb(${r}, ${g}, ${b})`;
            
            if (type === 'F') {
                return `<span style="color: ${color}">`;
            } else {
                return `<span style="background-color: ${color}">`;
            }
        });

        // Handle legacy color codes
        for (const [code, style] of Object.entries(colorMap)) {
            const regex = new RegExp(code.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g');
            
            if (code === '{@') {
                // Bold formatting
                result = result.replace(regex, '<strong>');
            } else if (code === '{!') {
                // Blink effect using CSS animation
                result = result.replace(regex, '<span class="blink">');
            } else if (code === '{+') {
                // Reverse effect
                result = result.replace(regex, '<span class="reverse">');
            } else if (code === '{x' || code === '{n') {
                // Reset formatting
                result = result.replace(regex, '</span>');
            } else if (code === '{{') {
                // Literal {
                result = result.replace(regex, '{');
            } else if (code === '{-') {
                // Literal ~
                result = result.replace(regex, '~');
            } else {
                // Color formatting
                const styleString = Object.entries(style)
                    .map(([key, value]) => `${key}: ${value}`)
                    .join('; ');
                result = result.replace(regex, `<span style="${styleString}">`);
            }
        }

        // Close any unclosed spans
        const openSpans = (result.match(/<span/g) || []).length;
        const closeSpans = (result.match(/<\/span>/g) || []).length;
        for (let i = 0; i < openSpans - closeSpans; i++) {
            result += '</span>';
        }

        return result;
    }

    // Utility function to apply color parsing to any text
    formatTextWithColors(text) {
        return this.parseColorCodes(text);
    }

    async init() {
        this.setupEventListeners();
        await this.loadItems();
        this.renderItems();
    }

    setupEventListeners() {
        const searchInput = document.getElementById('searchInput');
        const typeFilter = document.getElementById('typeFilter');

        searchInput.addEventListener('input', () => this.filterItems());
        typeFilter.addEventListener('change', () => this.filterItems());
    }

    async loadItems() {
        try {
            const response = await fetch('json/aether.json');
            const data = await response.json();
            
            // Filter items with at least 2 wear flags
            this.items = data.objects.filter(item => {
                if (!item.wear_flags) return false;
                const wearFlags = item.wear_flags.split(' ').filter(flag => flag.trim() !== '');
                return wearFlags.length >= 2;
            });

            this.filteredItems = [...this.items];
        } catch (error) {
            console.error('Error loading items:', error);
            document.getElementById('loading').innerHTML = '<p>Error loading items. Please check if the JSON file is accessible.</p>';
        }
    }

    filterItems() {
        const searchTerm = document.getElementById('searchInput').value.toLowerCase();
        const typeFilter = document.getElementById('typeFilter').value;

        this.filteredItems = this.items.filter(item => {
            const matchesSearch = item.name.toLowerCase().includes(searchTerm) ||
                                item.short_descr.toLowerCase().includes(searchTerm);
            const matchesType = !typeFilter || item.type === typeFilter;
            
            return matchesSearch && matchesType;
        });

        this.renderItems();
    }



    getRarityClass(item) {
        // Determine rarity based on level and number of affects
        const affectCount = item.affects ? item.affects.length : 0;
        
        if (item.level >= 120 || affectCount >= 8) return 'legendary';
        if (item.level >= 100 || affectCount >= 6) return 'epic';
        if (item.level >= 80 || affectCount >= 4) return 'rare';
        return '';
    }

    formatStatName(stat) {
        const statMap = {
            'hitroll': 'Hit Roll',
            'damroll': 'Damage Roll',
            'strength': 'Strength',
            'dexterity': 'Dexterity',
            'constitution': 'Constitution',
            'intelligence': 'Intelligence',
            'wisdom': 'Wisdom',
            'hp': 'Health',
            'mana': 'Mana',
            'move': 'Movement',
            'armor': 'Armor Class',
            'ac': 'AC',
            'saves': 'Saves',
            'resistance': 'Resistance',
            'crit_chance': 'Critical Chance',
            'crit_damage': 'Critical Damage',
            'penetration': 'Penetration',
            'insight': 'Insight',
            'celerity': 'Celerity',
            'recup': 'Recovery',
            'endurance': 'Endurance',
            'prosperity': 'Prosperity',
            'alacrity': 'Alacrity',
            'conc': 'Concentration'
        };
        
        return statMap[stat] || stat.charAt(0).toUpperCase() + stat.slice(1);
    }

    getStatClass(modifier, statName) {
        if (modifier === 0) return 'neutral';
        
        const positiveStats = ['hitroll', 'damroll', 'strength', 'dexterity', 'constitution', 
                             'intelligence', 'wisdom', 'hp', 'mana', 'move', 'resistance', 
                             'crit_chance', 'crit_damage', 'penetration', 'insight', 'celerity', 
                             'recup', 'endurance', 'prosperity', 'alacrity', 'conc'];
        
        const negativeStats = ['ac', 'saves'];
        
        if (positiveStats.includes(statName)) {
            return modifier > 0 ? 'positive' : 'negative';
        } else if (negativeStats.includes(statName)) {
            return modifier < 0 ? 'positive' : 'negative';
        }
        
        return modifier > 0 ? 'positive' : 'negative';
    }

    formatModifier(modifier) {
        if (modifier > 0) return `+${modifier}`;
        return modifier.toString();
    }

    renderItems() {
        const itemGrid = document.getElementById('itemGrid');
        const loading = document.getElementById('loading');
        const noResults = document.getElementById('noResults');

        loading.style.display = 'none';
        
        if (this.filteredItems.length === 0) {
            itemGrid.innerHTML = '';
            noResults.style.display = 'block';
            return;
        }

        noResults.style.display = 'none';
        
        itemGrid.innerHTML = this.filteredItems.map(item => this.createItemCard(item)).join('');
    }

    createItemCard(item) {
        const rarityClass = this.getRarityClass(item);
        const wearFlags = item.wear_flags ? item.wear_flags.split(' ').filter(flag => flag.trim() !== '') : [];
        
        let statsHTML = '';
        
        // Add affects/statistics
        if (item.affects && item.affects.length > 0) {
            item.affects.forEach(affect => {
                if (affect.type === 'normal' && affect.location && affect.modifier !== undefined) {
                    // Special handling for spellcast affects - show spell name instead of modifier
                    if (affect.location === 'spellcast' && affect.extra) {
                        statsHTML += `
                            <div class="stat-line epic">
                                <span>${this.formatTextWithColors('Spell')}</span>
                                <span>${this.formatTextWithColors(affect.extra)}</span>
                            </div>
                        `;
                    } else {
                        const statClass = this.getStatClass(affect.modifier, affect.location);
                        const formattedName = this.formatStatName(affect.location);
                        const formattedModifier = this.formatModifier(affect.modifier);
                        
                        statsHTML += `
                            <div class="stat-line ${statClass}">
                                <span>${this.formatTextWithColors(formattedName)}</span>
                                <span>${this.formatTextWithColors(formattedModifier)}</span>
                            </div>
                        `;
                    }
                } else if (affect.type === 'flag' && affect.location && affect.extra) {
                    // Handle special flags like shields and resists
                    const flagType = affect.location.split(':')[0];
                    let flagName = affect.extra;
                    
                    // Extract shield type name from "shield:typename" format
                    if (flagType === 'FS' && flagName.startsWith('shield:')) {
                        flagName = flagName.substring(7); // Remove "shield:" prefix
                    }
                    
                    // Extract weapon affect name from "affect2:typename" format
                    if (flagType === 'FB' && flagName.startsWith('affect2:')) {
                        flagName = flagName.substring(8); // Remove "affect2:" prefix
                    }
                    
                    // Extract affect name from "affect:typename" format
                    if (flagType === 'FA' && flagName.startsWith('affect:')) {
                        flagName = flagName.substring(7); // Remove "affect:" prefix
                    }
                    
                    // Extract resistance name from "resist:typename" format
                    if (flagType === 'FR' && flagName.startsWith('resist:')) {
                        flagName = flagName.substring(7); // Remove "resist:" prefix
                    }
                    
                    // Extract immunity name from "immune:typename" format (future use)
                    if (flagType === 'FI' && flagName.startsWith('immune:')) {
                        flagName = flagName.substring(7); // Remove "immune:" prefix
                    }
                    
                    // Extract vulnerability name from "vuln:typename" format (future use)
                    if (flagType === 'FV' && flagName.startsWith('vuln:')) {
                        flagName = flagName.substring(5); // Remove "vuln:" prefix
                    }
                    
                    if (flagType === 'FS') {
                        statsHTML += `
                            <div class="stat-line epic">
                                <span>${this.formatTextWithColors('Shield')}</span>
                                <span>${this.formatTextWithColors(flagName)}</span>
                            </div>
                        `;
                    } else if (flagType === 'FR') {
                        statsHTML += `
                            <div class="stat-line epic">
                                <span>${this.formatTextWithColors('Resistance')}</span>
                                <span>${this.formatTextWithColors(flagName)}</span>
                            </div>
                        `;
                    } else if (flagType === 'FA') {
                        statsHTML += `
                            <div class="stat-line epic">
                                <span>${this.formatTextWithColors('Affect')}</span>
                                <span>${this.formatTextWithColors(flagName)}</span>
                            </div>
                        `;
                    } else if (flagType === 'FB') {
                        statsHTML += `
                            <div class="stat-line epic">
                                <span>${this.formatTextWithColors('Affect')}</span>
                                <span>${this.formatTextWithColors(flagName)}</span>
                            </div>
                        `;
                    }
                } else if (affect.type === 'normal' && affect.location === 'spell_affect' && affect.extra) {
                    statsHTML += `
                        <div class="stat-line epic">
                            <span>${this.formatTextWithColors('Spell')}</span>
                            <span>${this.formatTextWithColors(affect.extra)}</span>
                        </div>
                    `;
                }
            });
        }

        // Add weapon stats if applicable
        if (item.values && item.values.weapon_type) {
            const damage = `${item.values.number_of_dice}d${item.values.type_of_dice}`;
            const damageType = item.values.damage_type || 'physical';
            
            statsHTML += `
                <div class="stat-line legendary">
                    <span>${this.formatTextWithColors('Damage')}</span>
                    <span>${this.formatTextWithColors(damage + ' ' + damageType)}</span>
                </div>
            `;
            
            // Add weapon type
            statsHTML += `
                <div class="stat-line rare">
                    <span>${this.formatTextWithColors('Weapon Type')}</span>
                    <span>${this.formatTextWithColors(item.values.weapon_type)}</span>
                </div>
            `;
        }

        // Add armor stats if applicable
        if (item.values && (item.values.ac_pierce || item.values.ac_bash || item.values.ac_slash)) {
            const acValues = [];
            if (item.values.ac_pierce) acValues.push(`Pierce: ${item.values.ac_pierce}`);
            if (item.values.ac_bash) acValues.push(`Bash: ${item.values.ac_bash}`);
            if (item.values.ac_slash) acValues.push(`Slash: ${item.values.ac_slash}`);
            if (item.values.ac_exotic) acValues.push(`Exotic: ${item.values.ac_exotic}`);
            
            if (acValues.length > 0) {
                statsHTML += `
                    <div class="stat-line rare">
                        <span>${this.formatTextWithColors('Armor Class')}</span>
                        <span>${this.formatTextWithColors(acValues.join(', '))}</span>
                    </div>
                `;
            }
        }

        // Add materia spell if applicable
        if (item.values && item.values.spell) {
            statsHTML += `
                <div class="stat-line rare">
                    <span>${this.formatTextWithColors('Junction')}</span>
                    <span>${this.formatTextWithColors(item.values.spell)}</span>
                </div>
            `;
        }

        // Create wear flags HTML
        const flagsHTML = wearFlags.map(flag => 
            `<span class="flag">${flag}</span>`
        ).join('');

        return `
            <div class="item-card ${rarityClass}" data-vnum="${item.vnum}">
                <div class="item-name">${this.formatTextWithColors(item.short_descr || item.name)}</div>
                <div class="item-type">${item.type}</div>
                
                ${statsHTML ? `<div class="item-stats">${statsHTML}</div>` : ''}
                
                ${flagsHTML ? `<div class="item-flags">${flagsHTML}</div>` : ''}
                
                ${item.description ? `<div class="item-description">${this.formatTextWithColors(item.description)}</div>` : ''}
            </div>
        `;
    }
}

// Initialize the application when the page loads
document.addEventListener('DOMContentLoaded', () => {
    new ItemViewer();
}); 