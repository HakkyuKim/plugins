# ubergeek77/tizen-studio-ide
# https://github.com/ubergeek77/Docker-tizen-studio-ide

FROM phusion/baseimage:0.10.2

# These are the IDs that will be used by the container. They MUST match the host UID/GID
# If these are not the same as your host user's IDs, please change them

ARG UID=1000
ARG GID=1000

# The non-root user that will be used to run Tizen Studio
ARG T_USER=tizen

# Add the non-root user to the system
RUN groupadd -g ${GID} -o ${T_USER} && \
	useradd -m -d /home/${T_USER} -u ${UID} -g ${GID} -o -s /bin/bash ${T_USER} && \
	mkdir -p /run/user/$UID && \
	chown ${UID}:${GID} /run/user/${UID}

# Install dependencies for Tizen Studio
RUN echo debconf shared/accepted-oracle-license-v1-1 select true | debconf-set-selections && \
	echo debconf shared/accepted-oracle-license-v1-1 seen true | debconf-set-selections && \
	add-apt-repository -y ppa:webupd8team/java && \
	apt-get update && \
	apt-get install -y \
		acl \
		bridge-utils \
		cpio \
		dbus-x11 \
		expect \
		gettext \
		libfontconfig1 \
		libglib2.0-0 \
		libjpeg-turbo8 \
		libpixman-1-0 \
		libpng12-0 \
		libsdl1.2debian \
		libsm6 \
		libv4l-0 \
		libwebkitgtk-1.0-0 \
		libx11-xcb1 \
		libxcb-icccm4 \
		libxcb-image0 \
		libxcb-randr0 \
		libxcb-render-util0 \
		libxcb-shape0 \
		libxcb-xfixes0 \
		libxi6 \
		make \
		openvpn \
		pciutils \
		python2.7 \
		libpython2.7 \
		rpm2cpio \
		ruby \
		zip \
		wget \
		git

# Download Tizen Studio installer
# If you want to download a different version of Tizen Studio, you may specify it in the argument below:
ARG T_VERSION=4.1
ARG T_BINARY=web-cli_Tizen_Studio_${T_VERSION}_ubuntu-64.bin
RUN wget http://download.tizen.org/sdk/Installer/tizen-studio_${T_VERSION}/${T_BINARY} \
	-O /home/${T_USER}/${T_BINARY} && \
	chown ${UID}:${GID} /home/${T_USER}/${T_BINARY} && \
	chmod +x /home/${T_USER}/${T_BINARY}

# Install Dotnet SDK
RUN wget https://packages.microsoft.com/config/ubuntu/18.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb && \
	dpkg -i packages-microsoft-prod.deb && \
	rm packages-microsoft-prod.deb && \
	apt-get update && \
	apt-get install -y apt-transport-https && \
	apt-get update && \
	apt-get install -y dotnet-sdk-5.0

RUN git clone https://github.com/flutter-tizen/flutter-tizen.git tools/flutter-tizen

# Clear the apt cache to reduce image size
RUN apt-get clean && rm -rf /var/lib/apt/lists/*

# Switch to the non-root user to run the installer
USER ${T_USER}

# Install Tizen Studio
RUN /home/${T_USER}/${T_BINARY} --accept-license /home/${T_USER}/tizen-studio && \
	/home/${T_USER}/tizen-studio/package-manager/package-manager-cli.bin install --accept-license \
	-d official \
	-s Tizen_Studio_${T_VERSION} \
	NativeToolchain-Gcc-9.2 \
    Baseline-SDK \
	WEARABLE-4.0-NativeAppDevelopment-CLI \
    cert-add-on \
    Certificate-Manager \
    WEARABLE-4.0-Emulator \
    WEARABLE-4.0-NativeAppDevelopment \
    WEARABLE-4.0 \
	AdvancedWEARABLE-4.0 \
    NativeIDE \
    WEARABLE-4.0-WebAppDevelopment-CLI \
    WEARABLE-4.0-WebAppDevelopment 

# Remove the installer file
RUN rm /home/${T_USER}/${T_BINARY}

# Switch back to the root user so that phusion's init scripts will work properly
USER root

# Set environment variables that persist when a container is run from the 
# resulting image.
ENV TIZEN_SDK=/home/tizen/tizen-studio
ENV FLUTTER_TIZEN=/tools/flutter-tizen/bin
ENV TIZEN_STUDIO_TOOLS=/home/tizen/tizen-studio/tools
ENV TIZEN_STUDIO_EMULATOR_CLI=/home/tizen/tizen-studio/tools/emulator/bin

# Update PATH
ENV PATH=$TIZEN_SDK:$FLUTTER_TIZEN:$TIZEN_STUDIO_TOOLS:$TIZEN_STUDIO_EMULATOR_CLI:$PATH