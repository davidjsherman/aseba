#!groovy

// Jenkinsfile for compiling, testing, and packaging Aseba.
// Requires CMake plugin from https://github.com/davidjsherman/aseba-jenkins.git in global library.

// Ideally we should unarchive previously compiled Dashel and Enki libraries, but for
// now we conservatively recompile them.
// (possible example in https://issues.jenkins-ci.org/browse/JENKINS-32780)

pipeline {
	agent label:'debian' // use any available Jenkins agent

	parameters {
        stringParam(defaultValue: 'pollsocketstream', description: 'Dashel branch', name: 'branch_dashel')
        stringParam(defaultValue: 'alpha-classcode-jenkins', description: 'Enki branch', name: 'branch_enki')
		booleanParam(defaultValue: false, description: 'Build packages', name: 'do_packaging')
    }

	stages {
		stage('Prepare') {
			steps {
				sh 'mkdir -p externals build dist'
				dir('aseba') {
					checkout scm
					sh 'git submodule update --init'
				}
				dir('externals/dashel') {
					git branch: "${env.branch_dashel}", url: 'https://github.com/davidjsherman/dashel.git'
				}
				dir('externals/enki') {
					git branch: "${env.branch_enki}", url: 'https://github.com/davidjsherman/enki.git'
				}
				stash excludes: '.git', name: 'source'
				echo "branch_dashel=${env.branch_dashel}"
				echo "branch_enki=${env.branch_enki}"
				echo "do_packaging=${env.do_packaging}"
			}
		}
		stage('Dashel') {
			steps {
				unstash 'source'
				CMake([buildType: 'Debug',
					   sourceDir: '$workDir/externals/dashel',
					   buildDir: '$workDir/build/dashel',
					   installDir: '$workDir/dist',
					   getCmakeArgs: [ '-DBUILD_SHARED_LIBS:BOOL=ON' ]
					  ])
			}
			post {
				always {
					stash includes: 'dist/**', name: 'dashel'
				}
			}
		}
		stage('Enki') {
			steps {
				unstash 'source'
				CMake([buildType: 'Debug',
					   sourceDir: '$workDir/externals/enki',
					   buildDir: '$workDir/build/enki',
					   installDir: '$workDir/dist',
					   getCmakeArgs: [ '-DBUILD_SHARED_LIBS:BOOL=ON' ]
					  ])
			}
			post {
				always {
					stash includes: 'dist/**', name: 'enki'
				}
			}
		}
		stage('Compile') {
			steps {
				unstash 'dashel'
				unstash 'enki'
				unstash 'source'
				script {
					env.dashel_DIR = sh ( script: 'dirname $(find dist -name dashelConfig.cmake | head -1)', returnStdout: true).trim()
				}
				CMake([buildType: 'Debug',
					   sourceDir: '$workDir/aseba',
					   buildDir: '$workDir/build/aseba',
					   installDir: '$workDir/dist',
					   envs: [ "dashel_DIR=${env.dashel_DIR}" ]
					  ])
			}
			post {
				always {
					stash includes: 'dist/**', name: 'aseba'
				}
			}
		}
		stage('Test') {
			steps {
				unstash 'aseba'
				dir('build/aseba') {
					sh "ctest -E 'e2e.*|simulate.*|.*http.*|valgrind.*'"
				}
			}
		}
		stage('Package') {
			when {
				env.do_packaging == 'true' && sh(script:'which debuild', returnStatus: true) == 0
			}
			steps {
				unstash 'source'
				sh 'cd aseba && debuild -i -us -uc -b'
				sh 'mv aseba*.deb aseba*.changes aseba*.build dist/'
			}
		}
		stage('Archive') {
			steps {
				archiveArtifacts artifacts: 'dist/**', fingerprint: true, onlyIfSuccessful: true
			}
		}
	}
}
