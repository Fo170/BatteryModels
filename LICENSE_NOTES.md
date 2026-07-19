# ⚖️ NOTES LICENCE - GNU GPLv3

**Date**: 2026-07-19  
**Licence**: GNU General Public License v3.0  
**Status**: En vigueur (changement de MIT vers GPLv3)

---

## 📋 Résumé GPLv3

BatteryModels est désormais distribué sous **GNU GPLv3**, une licence copyleft forte.

### ✅ Vous POUVEZ:

- ✓ Utiliser la bibliothèque dans votre projet
- ✓ Modifier le code source
- ✓ Distribuer le code modifié
- ✓ Utiliser commercialement
- ✓ Créer des dérivés

### ⚠️ VOUS DEVEZ:

**Si vous distribuez un projet utilisant BatteryModels (modifié ou non):**

1. **Inclure la licence GPLv3** dans votre distribution
2. **Fournir le code source** (ou un lien pour l'obtenir)
3. **Documenter les modifications** effectuées sur BatteryModels
4. **Appliquer GPLv3** à votre projet entier (ou au moins la partie qui utilise BatteryModels)

### ❌ Restrictions GPLv3:

- ❌ Vous NE pouvez PAS utiliser une licence plus permissive (MIT, Apache, BSD)
- ❌ Vous NE pouvez PAS garder le code privé si vous le distribuez
- ❌ Vous NE pouvez PAS passer sous propriétaire

---

## 🔍 IMPLICATIONS PRATIQUES

### **Cas 1: Projet Arduino Local (Recommandé)**

```
Votre sketch.ino
  ↓
#include <BatteryModels.h>
  ↓
Compilé localement
  ↓
Téléchargé sur votre Arduino
```

✅ **LÉGAL sans restrictions additionnelles**
- Vous pouvez garder votre sketch privé
- Aucune obligation de partager

---

### **Cas 2: Publication sur GitHub (OBLIGATION GPLv3)**

```
Votre projet GitHub
  ↓
├─ src/
│  ├─ sketch.ino (VOTRE CODE)
│  └─ BatteryModels.h (EMPRUNTÉ)
├─ LICENSE (DOIT ÊTRE GPLv3)
└─ README.md (DOIT MENTIONNER GPLv3)
```

⚠️ **OBLIGATOIRE**:
- Inclure copie de LICENSE (GPLv3)
- Mentionner "Uses BatteryModels (GPLv3)"
- Votre projet ENTIER devient GPLv3
- Tout dérivé doit aussi être GPLv3

**Exemple README.md minimale**:
```markdown
## Licence

Ce projet utilise [BatteryModels](https://github.com/Fo170/BatteryModels)
distribué sous GNU GPLv3.

Voir [LICENSE](LICENSE) pour les détails.
```

---

### **Cas 3: Produit Propriétaire Commercial**

```
Votre produit commercial (firmware fermé)
  ↓
Utilise BatteryModels
  ↓
???
```

❌ **PAS PERMIS sans exception**:
- GPLv3 oblige à partager le code
- Vous NE pouvez PAS garder firmware propriétaire

**Solutions**:
1. **Licence explicite du propriétaire** (Olivier Fournet)
   - Demander une licence commerciale/propriétaire
   - Possiblement payante

2. **Refactoriser sans BatteryModels**
   - Coder votre propre modèle batterie
   - Respecter GPLv3 seulement pour vos emprunts réels

3. **Open-source le projet**
   - Publier sous GPLv3
   - Acceptable pour bien des applications

---

### **Cas 4: Intégration dans Bibliothèque Arduino**

```
Votre lib Arduino
  ↓
uses BatteryModels (GPLv3)
  ↓
Autre sketch utilise votre lib
  ↓
Tout doit être GPLv3
```

⚠️ **OBLIGATION DE CASCADE**:
- Votre lib devient GPLv3
- Tout projet utilisant votre lib doit être GPLv3
- Cela peut affecter vos utilisateurs

**Important**: Documentez clairement la dépendance GPLv3!

---

## 📝 COMPATIBILITÉ AVEC AUTRES LICENCES

| Licence | Compatible avec GPLv3 | Raison |
|---------|---|---|
| GPLv3 | ✅ OUI | Même licence |
| GPLv2 | ⚠️ PARTIEL | Peut avoir problèmes |
| AGPL v3 | ✅ OUI | Compatible forte |
| MIT | ❌ NON | Trop permissive, conflit copyleft |
| Apache 2.0 | ❌ NON | Idem MIT |
| BSD | ❌ NON | Idem MIT |
| Propriétaire | ❌ NON | Absolutement interdit |

**Règle d'or**: Si GPLv3 + X, alors résultat = GPLv3 (ou plus strict)

---

## 🛡️ PROTECTION LÉGALE

### **Votre Code est Protégé:**
- Vous retenez le copyright
- GPLv3 est un contrat d'utilisation
- Toute violation peut être poursuivie légalement

### **Dérivés Doivent Être Partagés:**
- Quelqu'un modifie BatteryModels et le vend = **violation**
- Quelqu'un modifie BatteryModels en fermé = **violation**
- Quelqu'un le revendique comme sien = **violation**

---

## 🔄 TRANSITION MIT → GPLv3

**Avant (v1.0.0)**:
```json
"license": "MIT"
```
- Plus permissive
- Pas obligation partage
- Pas obligation attribution

**Après (v1.0.1+)**:
```json
"license": "GPL-3.0-only"
```
- Copyleft fort
- Obligation partage + attribution
- Tout dérivé publié doit être GPLv3

### **Impact sur les Utilisateurs Existants:**

- **Code local** (non distribué): ✅ OK, pas d'impact
- **Distribué en MIT avant**: ⚠️ Peut continuer en MIT (héritage)
- **Code nouveau**: ⚠️ Doit respecter GPLv3

---

## 📞 QUESTIONS FRÉQUENTES

### **Q: Je peux utiliser BatteryModels en commercial?**
✅ **OUI**, mais:
- Vous devez fournir le code source
- Le projet entier doit être GPLv3 compatible
- Ou obtenir une licence alternative du propriétaire

### **Q: Dois-je payer pour utiliser?**
✅ **NON**, BatteryModels est gratuit
- Licence libre et open-source

### **Q: Je peux modifier et garder privé?**
✅ **OUI**, tant que vous ne distribuez pas
- Modifications locales: OK
- Publication: Obligation partage

### **Q: Quelle est la différence MIT → GPLv3?**

| Aspect | MIT | GPLv3 |
|--------|-----|-------|
| Commercial? | ✅ Oui | ✅ Oui |
| Modifications? | ✅ Oui | ✅ Oui |
| Privé local? | ✅ Oui | ✅ Oui |
| Distribution privée? | ✅ Oui, pas de restriction | ❌ Non, copyleft |
| Obligation source? | ❌ Non | ✅ Oui |
| Dérivé doit être GPLv3? | ❌ Non | ✅ Oui |

### **Q: Puis-je utiliser en projet propriétaire?**
- ❌ Pas sans accord spécial
- ⚠️ Contactez l'auteur: olivier.fournet@free.fr
- Peut être négocié pour usage commercial

---

## ✉️ CONTACT POUR EXCEPTIONS

Pour une licence alternative ou propriétaire:

**Auteur**: Olivier Fournet  
**Email**: olivier.fournet@free.fr  
**GitHub**: https://github.com/Fo170/

---

## 📚 RESSOURCES

- **GPLv3 Texte complet**: https://www.gnu.org/licenses/gpl-3.0.html
- **FAQ GPLv3**: https://www.gnu.org/licenses/gpl-faq.html
- **Compatibilité licences**: https://en.wikipedia.org/wiki/License_compatibility

---

## ⚖️ CLAUSE DE NON-RESPONSABILITÉ

BatteryModels est fourni "AS IS" sans garantie.

**Voir LICENSE pour les détails complets légaux.**

Cet document est un résumé informatif. Pour les termes légaux exacts, consultez le fichier LICENSE ou un avocat.

---

**Date**: 2026-07-19  
**Version**: GPLv3  
**Status**: En vigueur
