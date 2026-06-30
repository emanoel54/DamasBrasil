#!/bin/bash

cp -r /opt/patriotas/data/ /home/emanoel/.

# Configurações do pacote
PKG_NAME="patriotas"
VERSION="1.1"
ARCH="amd64"
BUILD_DIR="${PKG_NAME}_${VERSION}_${ARCH}"

echo "=> Limpando empacotamentos antigos..."
rm -rf ${BUILD_DIR}
rm -f ${BUILD_DIR}.deb

echo "=> Criando arvore de diretorios do pacote..."
mkdir -p ${BUILD_DIR}/DEBIAN
mkdir -p ${BUILD_DIR}/opt/patriotas/data
mkdir -p ${BUILD_DIR}/opt/patriotas/img
mkdir -p ${BUILD_DIR}/opt/patriotas/audio
mkdir -p ${BUILD_DIR}/usr/share/applications
mkdir -p ${BUILD_DIR}/usr/share/icons/hicolor/256x256/apps

echo "=> Criando arquivo de controle (metadata)..."
cat <<EOF > ${BUILD_DIR}/DEBIAN/control
Package: patriotas
Version: ${VERSION}
Section: games
Priority: optional
Architecture: ${ARCH}
Depends: libgtk-3-0, libgtkmm-3.0-1v5, libzstd1, alsa-utils
Maintainer: Emanoel Libonati <brasillinux20@gmail.com>
Description: Patriotas - Motor de Jogo de Damas Brasileiro
 Interface e motor IA para o Jogo de Damas na regra brasileira (64 casas).
 Possui aprendizado continuo (Reinforcement Learning) e Base de Finais.
EOF

echo "=> Criando script postinst (Libera acesso as pastas sem Root)..."
cat <<'EOF' > ${BUILD_DIR}/DEBIAN/postinst
#!/bin/bash
# Da permissao total (777) as pastas de dados apos instalar.
# Assim a IA pode evoluir e salvar seus txts sem erro de permissao.
chmod -R 777 /opt/patriotas/data 2>/dev/null || true
chmod -R 777 /opt/patriotas/db 2>/dev/null || true
exit 0
EOF
chmod 755 ${BUILD_DIR}/DEBIAN/postinst

echo "=> Copiando o projeto compilado e assets..."
cp build/patriotas ${BUILD_DIR}/opt/patriotas/
cp data/* ${BUILD_DIR}/opt/patriotas/data/ 2>/dev/null || true
cp img/patriotas.png ${BUILD_DIR}/opt/patriotas/img/ 2>/dev/null || true
cp img/patriotas.png ${BUILD_DIR}/usr/share/icons/hicolor/256x256/apps/ 2>/dev/null || true
cp audio/move.wav ${BUILD_DIR}/opt/patriotas/audio/ 2>/dev/null || true

echo "=> Criando atalho de menu do Linux (.desktop)..."
cat <<EOF > ${BUILD_DIR}/usr/share/applications/patriotas.desktop
[Desktop Entry]
Version=${VERSION}
Name=Patriotas
Comment=Motor de Damas Brasileiro IA
Exec=bash -c "/opt/patriotas/patriotas > /dev/null 2>&1"
Path=/opt/patriotas
Icon=patriotas
Terminal=false
Type=Application
Categories=Game;BoardGame;
EOF

echo "=> Empacotando com dpkg-deb..."
dpkg-deb --build ${BUILD_DIR}

echo "=> Concluido! O pacote ${BUILD_DIR}.deb foi gerado na sua pasta."
echo "Para instala-lo no seu sistema, digite: sudo apt install ./${BUILD_DIR}.deb"
